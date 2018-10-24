//module.exports = require(__dirname + '/build/Release/nodeyolo').Yolo;

let yolo_addon = require(__dirname + '/build/Debug/nodeyolo').Yolo;

let obj = new yolo_addon("./darknet", "./cfg/coco.data", "./cfg/yolov3-spp.cfg", "../weights/yolov3-spp.weights");

// obj.detectImage("./darknet/data/kite.jpg")
//     .then((detections) => {
//         console.log(JSON.stringify(detections));
//     })
//     .catch((error) => {
//         console.error(error);
//     });

obj.detectVideo("./data/crowd.mp4")
    .then((detections) => {
        console.log(JSON.stringify(detections));
    })
    .catch((error) => {
        console.error(error);
    });