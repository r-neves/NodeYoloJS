#include "libyolo.h"
#include <limits.h>
#include <pthread.h>
#include <fcntl.h>
#include <opencv2/imgcodecs/imgcodecs_c.h>

void fill_detect(yolo_object *yolo, detection *network_detection, int network_detection_index, detect *yolo_detect)
{
 size_t strlength=strlen(yolo->names[network_detection_index]);
 yolo->names[network_detection_index][strlength]='\0';
 yolo_detect->class_name=calloc(strlength+1, sizeof(char));
 strcpy(yolo_detect->class_name, yolo->names[network_detection_index]);
 yolo_detect->probability=network_detection->prob[network_detection_index]*100;
 yolo_detect->bbox=network_detection->bbox;
 box *bbox=&yolo_detect->bbox;
 bbox->x=bbox->x-(bbox->w/2);
 bbox->y=bbox->y-(bbox->h/2);
 if(bbox->x<0)
 {
  bbox->x=0;
 }
 if(bbox->y<0)
 {
  bbox->y=0;
 }
}

yolo_status fill_detection(yolo_object *yolo, detection *dets, yolo_detection_image *yolo_detect, float time_spent_for_classification, int nboxes, float thresh)
{
 yolo_detect->time_spent_for_classification=time_spent_for_classification;
 yolo_detect->num_boxes=0;
 int class_index;
 detection *det;

 for(int i=0; i<nboxes; ++i)
 {
  class_index=-1;
  det=NULL;
  for(int j=0; j<dets[i].classes; ++j)
  {
   if(dets[i].prob[j]>=thresh)
   {
    if(det == NULL || (dets[i].prob[j]>det->prob[class_index]))
    {
     class_index=j;
     det=dets+i;
    }
   }
  }
  if(class_index>-1 && det != NULL)
  {
   void *temp_pointer=realloc(yolo_detect->detection, sizeof(detect)*(yolo_detect->num_boxes+1));
   if(temp_pointer == NULL)
   {
    return yolo_cannot_realloc_detect;
   }
   yolo_detect->detection=temp_pointer;
   fill_detect(yolo, det, class_index, yolo_detect->detection+yolo_detect->num_boxes);
   yolo_detect->num_boxes++;
  }
 }

 return yolo_ok;
}

yolo_status parse_detections_image(yolo_object *yolo, detection *dets, yolo_detection_image **yolo_detect, float time_spent_for_classification, int nboxes, float thresh)
{
 if((*yolo_detect) == NULL)
 {
  (*yolo_detect)=calloc(1, sizeof(yolo_detection_image));
  if((*yolo_detect) == NULL)
  {
   return yolo_cannot_alloc_yolo_detection;
  }
 }

 return fill_detection(yolo,dets,(*yolo_detect),time_spent_for_classification,nboxes,thresh);
}

yolo_status parse_detections_video(yolo_object *yolo, detection *dets, yolo_detection_video **yolo_detect, float time_spent_for_classification, long frame_id, double milisecond, int nboxes, float thresh)
{
 if((*yolo_detect) == NULL)
 {
  (*yolo_detect)=calloc(1, sizeof(yolo_detection_video*));
  if((*yolo_detect) == NULL)
  {
   return yolo_cannot_alloc_yolo_detection;
  }
 }
 yolo_detection_video *video_detection=*yolo_detect;
 yolo_detection_frame *temp=realloc(video_detection->frame_detections, sizeof(yolo_detection_frame)*(video_detection->count+1));
 if(temp==NULL)
 {
  return yolo_cannot_alloc_yolo_detection;
 }
 memset(temp+video_detection->count,0,sizeof(yolo_detection_frame));
 video_detection->frame_detections=temp;
 video_detection->frame_detections[video_detection->count].frame=frame_id;
 video_detection->frame_detections[video_detection->count].milisecond=milisecond;
 yolo_status yolo_stats=fill_detection(yolo,dets,&video_detection->frame_detections[video_detection->count].detection_frame,time_spent_for_classification,nboxes,thresh);
 ++video_detection->count;
 return yolo_stats;
}

void *thread_detect(void *data)
{
 if(data == NULL)
 {
  return NULL;
 }
 thread_data_t *th_data=data;
 while(true)
 {
  if(sem_trywait(th_data->full))
  {
   bool end;
   if(pthread_mutex_lock(&th_data->mutex_end)!=0)
   {
    continue;
   }
   end=th_data->end;
   pthread_mutex_unlock(&th_data->mutex_end);
   if(end)
   {
    break;
   }
   continue;
  }

  image im;
  long frame_number;
  double milisecond;

  bool im_got_sucessfull;
  if(pthread_mutex_lock(&th_data->mutex_stack))
  {
   continue;
  }
  im_got_sucessfull=stack_pop(&th_data->stack,&im,&frame_number,&milisecond);
  pthread_mutex_unlock(&th_data->mutex_stack);
  sem_post(th_data->empty);

  if(!im_got_sucessfull)
  {
   continue;
  }

  layer l=th_data->yolo->net->layers[th_data->yolo->net->n-1];
  clock_t time;
  float nms=0.45;

  image sized=resize_image(im, th_data->yolo->net->w, th_data->yolo->net->h);
  float *X=sized.data;
  time=clock();
  network_predict(th_data->yolo->net, X);

  int nboxes=0;
  detection *dets=get_network_boxes(th_data->yolo->net, im.w, im.h, th_data->thresh, 0, 0, 0, &nboxes);
  if(nms)
  {
   do_nms_sort(dets, l.side*l.side*l.n, l.classes, nms);
  }

  parse_detections_video(th_data->yolo, dets, th_data->yolo_detect, sec(clock()-time), frame_number, milisecond, nboxes, th_data->thresh);
  free_detections(dets, nboxes);
  free_image(im);
  free_image(sized);
 }
 return NULL;
}

yolo_status yolo_check_before_process_filename(yolo_object *yolo, char *filename)
{
 if(yolo == NULL)
 {
  return yolo_object_is_not_initialized;
 }

 if(access(filename, F_OK) == -1)
 {
  fprintf(stderr, "error yolo_detect: %s\n", strerror(errno));
  return yolo_image_file_is_not_exists;
 }

 if(access(filename, R_OK) == -1)
 {
  fprintf(stderr, "error yolo_detect: %s\n", strerror(errno));
  return yolo_image_file_is_not_readable;
 }
 return yolo_ok;
}

void yolo_cleanup(yolo_object *yolo)
{
 if(yolo == NULL)
 {
  return;
 }
 free_network(yolo->net);
 if(yolo->names != NULL)
 {
  for(int i=0; i<yolo->class_number; i++)
  {
   if(yolo->names[i] != NULL)
   {
    free(yolo->names[i]);
   }
  }
  free(yolo->names);
 }
 free(yolo);
 yolo_object **ptr_yolo=&yolo;
 (*ptr_yolo)=NULL;
}

yolo_status yolo_init(yolo_object **yolo_obj, char *workingDir, char *datacfg, char *cfgfile, char *weightfile)
{
 clock_t time=clock();

 yolo_cleanup((*yolo_obj));

 (*yolo_obj)=(yolo_object *)malloc(sizeof(yolo_object));

 yolo_object *yolo=(*yolo_obj);
 if(!yolo)
 {
  return yolo_cannot_alloc_node_yolo_object;
 }
 memset(yolo, 0, sizeof(yolo_object));

 if(access(workingDir, F_OK) == -1)
 {
  fprintf(stderr, "error yolo_init: %s\n", strerror(errno));
  return yolo_working_dir_is_not_exists;
 }

 if(access(workingDir, R_OK) == -1)
 {
  fprintf(stderr, "error yolo_init: %s\n", strerror(errno));
  return yolo_working_dir_is_not_readable;
 }
 char cur_dir[1024];
 getcwd(cur_dir, sizeof(cur_dir));
 if(chdir(workingDir) == -1)
 {
  fprintf(stderr, "%s\n", strerror(errno));
  return yolo_cannot_change_to_working_dir;
 }

 if(access(cfgfile, F_OK) == -1)
 {
  fprintf(stderr, "error yolo_init: %s\n", strerror(errno));
  return yolo_cfgfile_is_not_exists;
 }
 if(access(cfgfile, R_OK) == -1)
 {
  fprintf(stderr, "error yolo_init: %s\n", strerror(errno));
  return yolo_cfgfile_is_not_readable;
 }
 if(access(weightfile, F_OK) == -1)
 {
  fprintf(stderr, "error yolo_init: %s\n", strerror(errno));
  return yolo_weight_file_is_not_exists;
 }
 if(access(weightfile, R_OK) == -1)
 {
  fprintf(stderr, "error yolo_init: %s\n", strerror(errno));
  return yolo_weight_file_is_not_readable;
 }
 yolo->net=load_network(cfgfile, weightfile, 0);

 if(access(datacfg, F_OK) == -1)
 {
  fprintf(stderr, "error yolo_init: %s\n", strerror(errno));
  return yolo_datacfg_is_not_exists;
 }

 if(access(datacfg, R_OK) == -1)
 {
  fprintf(stderr, "error yolo_init: %s\n", strerror(errno));
  return yolo_datacfg_is_not_readable;
 }
 list *options=read_data_cfg(datacfg);
 char *name_list=option_find_str(options, "names", "data/names.list");

 if(access(name_list, F_OK) == -1)
 {
  fprintf(stderr, "error yolo_init: %s\n", strerror(errno));
  return yolo_names_file_is_not_exists;
 }
 if(access(name_list, R_OK) == -1)
 {
  fprintf(stderr, "error yolo_init: %s\n", strerror(errno));
  return yolo_names_file_is_not_readable;
 }
 yolo->names=get_labels(name_list);
 char *classes=option_find_str(options, "classes", "data/names.list");
 char *bad_ptr=NULL;
 long value=strtol(classes, &bad_ptr, 10);
 if(value<INT_MAX)
 {
  yolo->class_number=(int)value;
 }

 set_batch_network(yolo->net, 1);
 srand(2222222);

 printf("Network configured and loaded in %f seconds\n", sec(clock()-time));
 chdir(cur_dir);
 return yolo_ok;
}

yolo_status yolo_detect_image(yolo_object *yolo, yolo_detection_image **detect, char *filename, float thresh)
{
 yolo_status status=yolo_check_before_process_filename(yolo, filename);
 if(status != yolo_ok)
 {
  return status;
 }

 CvMat *mat=cvLoadImageM(filename, CV_LOAD_IMAGE_COLOR);
 if(mat == NULL)
 {
  fprintf(stderr, "error yolo_detect: %s\n", strerror(errno));
  return yolo_image_file_is_corrupted;
 }
 cvReleaseMat(&mat);

 layer l=yolo->net->layers[yolo->net->n-1];
 clock_t time;
 float nms=0.45;

 image im=load_image_color(filename, 0, 0);
 image sized=resize_image(im, yolo->net->w, yolo->net->h);
 float *X=sized.data;
 time=clock();
 network_predict(yolo->net, X);

 int nboxes=0;
 detection *dets=get_network_boxes(yolo->net, im.w, im.h, thresh, 0.5, 0, 0, &nboxes);
 if(nms)
 {
  do_nms_sort(dets, l.side*l.side*l.n, l.classes, nms);
 }

 status=parse_detections_image(yolo, dets, detect, sec(clock()-time), nboxes, thresh);
 if(status != yolo_ok)
 {
  return status;
 }

 free_detections(dets, nboxes);
 free_image(im);
 free_image(sized);

 return yolo_ok;
}

void yolo_detect_free(yolo_detection_image *yolo_det)
{
 for(int i=0; i<yolo_det->num_boxes; i++)
 {
  free(yolo_det->detection[i].class_name);
 }
 free(yolo_det->detection);
}

void yolo_detection_image_free(yolo_detection_image **yolo)
{
 if((*yolo) == NULL)
 {
  return;
 }
 yolo_detect_free(*yolo);
 free(*yolo);
 (*yolo)=NULL;
}

void yolo_detection_video_free(yolo_detection_video **yolo)
{
 yolo_detection_video *yolo_det=*yolo;
 if(yolo_det == NULL)
 {
  return;
 }
 for(int i=0; i<yolo_det->count; ++i)
 {
  yolo_detect_free(&yolo_det->frame_detections[i].detection_frame);
 }
 free(yolo_det->frame_detections);
 free(yolo_det);
 (*yolo)=NULL;
}

yolo_status_detailed yolo_status_decode(yolo_status status)
{
 yolo_status_detailed status_detailed;
 status_detailed.error_code=status;
 switch(status)
 {
  case yolo_instanciation:
   status_detailed.error_message="Cannot instantiate due an error.";
   break;
  case yolo_cannot_realloc_detect:
   status_detailed.error_message="Cannot allocate detect in memory";
   break;
  case yolo_cannot_alloc_yolo_detection:
   status_detailed.error_message="Cannot allocate yolo_detection in memory";
   break;
  case yolo_cannot_alloc_node_yolo_object:
   status_detailed.error_message="Cannot allocate node_yolo_object in memory";
   break;
  case yolo_cannot_alloc_map:
   status_detailed.error_message="Cannot allocate map in memory";
   break;
  case yolo_cannot_change_to_working_dir:
   status_detailed.error_message="Cannot change to working directory";
   break;
  case yolo_object_is_not_initialized:
   status_detailed.error_message="yolo_object isn't allocated in memory";
   break;
  case yolo_working_dir_is_not_exists:
   status_detailed.error_message="working directory don't exists";
   break;
  case yolo_datacfg_is_not_exists:
   status_detailed.error_message="datacfg don't exists";
   break;
  case yolo_cfgfile_is_not_exists:
   status_detailed.error_message="cfgfile don't exists";
   break;
  case yolo_weight_file_is_not_exists:
   status_detailed.error_message="weight file don't exists";
   break;
  case yolo_working_dir_is_not_readable:
   status_detailed.error_message="working directory isn't readable";
   break;
  case yolo_datacfg_is_not_readable:
   status_detailed.error_message="datacfg isn't readable";
   break;
  case yolo_cfgfile_is_not_readable:
   status_detailed.error_message="cfgfile isn't readable";
   break;
  case yolo_weight_file_is_not_readable:
   status_detailed.error_message="weight file isn't readable";
   break;
  case yolo_names_file_is_not_exists:
   status_detailed.error_message="names file don't exists";
   break;
  case yolo_names_file_is_not_readable:
   status_detailed.error_message="names file isn't readable";
   break;
  case yolo_image_file_is_not_exists:
   status_detailed.error_message="image file isn't exists";
   break;
  case yolo_image_file_is_not_readable:
   status_detailed.error_message="image file isn't readable";
   break;
  case yolo_image_file_is_corrupted:
   status_detailed.error_message="image file is corrupted";
   break;
   //  case yolo_napi_create_object_time_spent_for_classification_double_failed:
   //   status_detailed.error_message="image file is corrupted";
   //   break;
   //  case yolo_napi_create_object_time_spent_for_classification_named_property_failed:
   //   status_detailed.error_message="image file is corrupted";
   //   break;
   //  case yolo_napi_set_array_property_failed:
   //   status_detailed.error_message="image file is corrupted";
   //   break;
   //  case yolo_napi_create_main_object_failed:
   //   status_detailed.error_message="image file is corrupted";
   //   break;
  default:
   status_detailed.error_message="Unknow error";
 }
 return status_detailed;
}