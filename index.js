let yolo_addon = require('bindings')('nodeyolojs');

// module.exports=yolo_addon.Yolo;

//let obj = new yolo_addon.Yolo("./darknet", "./cfg/coco.data", "./cfg/yolov2-tiny.cfg", "../yolov2-tiny.weights");
let obj1 = new yolo_addon.Yolo("./darknet", "./cfg/coco.data", "./cfg/yolov3.cfg", "../yolov3-416.weights");

// obj1.detect("darknet/data/horses.jpg")
//  .then((detections) => {
//   console.log(1);
//  })
//  .catch((error) => {
//   console.error(error);
//  });
//
// obj1.detect("darknet/data/dog.jpg")
//  .then((detections) => {
//   console.log(2);
//  })
//  .catch((error) => {
//   console.error(error);
//  });

const memwatch = require('memwatch-next');

memwatch.on('leak', (info) => {
 console.error('Memory leak detected:\n', info);
});

['dog', 'eagle', 'giraffe', 'horses', 'kite', 'person', 'scream'].forEach((img) => {
 obj1.detect("darknet/data/" + img + ".jpg")
  .then((detections) => {
   console.log(img + "\n" + JSON.stringify(detections));
  })
  .catch((error) => {
   console.error(error);
  });
});