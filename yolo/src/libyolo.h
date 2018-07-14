#ifndef LIBYOLO_H
#define LIBYOLO_H

#include "darknet.h"
#include <unistd.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct
{
 char *class_name;
 float probability;
 box bbox;
}detect;

typedef struct
{
 detect *detection;
 int num_boxes;
}yolo_detection;

typedef struct
{
 int class_number;
 char **names;
 float nms;
 network *net;
}yolo_object;

typedef struct
{
 int error_code;
 char *error_message;
}yolo_status_detailed;

typedef enum
{
 yolo_ok,
 yolo_cannot_alloc_node_yolo_object,
 yolo_cannot_alloc_map,
 yolo_cannot_alloc_yolo_detection,
 yolo_cannot_alloc_detect,
 yolo_cannot_change_to_working_dir,
 yolo_object_is_not_initialized,
 yolo_working_dir_is_not_exists,
 yolo_datacfg_is_not_exists,
 yolo_cfgfile_is_not_exists,
 yolo_weight_file_is_not_exists,
 yolo_working_dir_is_not_readable,
 yolo_datacfg_is_not_readable,
 yolo_cfgfile_is_not_readable,
 yolo_weight_file_is_not_readable,
 yolo_names_file_is_not_exists,
 yolo_names_file_is_not_readable,
 yolo_image_file_is_not_exists,
 yolo_image_file_is_not_readable,
}yolo_status;

yolo_status yolo_init(yolo_object **yolo_obj, char *workingDir, char *datacfg, char *cfgfile, char *weightfile);

yolo_status yolo_detect(yolo_object *yolo, yolo_detection **detect, char *filename, float thresh);

void yolo_detection_free(yolo_detection **yolo);
void yolo_cleanup(yolo_object *yolo);

yolo_status_detailed yolo_status_decode(yolo_status status);

#ifdef __cplusplus
};
#endif

#endif // LIBYOLO_H