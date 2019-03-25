import QtQuick 2.9
import QtQuick.Controls 2.2
import QtMultimedia 5.11

import ObjectsRecognizer 1.0
import AuxUtils 1.0

Page {
    id: root
    title: qsTr("Live")

    readonly property double fontPixelSize: Qt.application.font.pixelSize * 1.6
    readonly property var    defResolutions: ["320x240","640x480","800x480","800x600","1024x768","1280x720","1920x1080"]

    property double minConfidence
    property int    nThreads
    property bool   showInfTime
    property string resolution
    property var    resolutions: []
    property var    tfObjects

    onResolutionChanged: auxUtils.setResolution(resolution)

    background: Rectangle { color: 'black'}

    signal cameraResolutionsLoaded()

    Camera {
        id: camera
        deviceId: QtMultimedia.defaultCamera.deviceId
        captureMode: Camera.CaptureViewfinder
        property bool availableCamera:  QtMultimedia.availableCameras.length>0
        viewfinder.resolution: resolution

        onCameraStateChanged: {
            if (camera.cameraState === 2)
            {
                var res  = camera.supportedViewfinderResolutions()

                if (res.length>0)
                {
                    resolutions = []

                    for(var i=0; i<res.length; i++)
                        resolutions.push(res[i].width + 'x' + res[i].height)
                }
                else resolutions = defResolutions
            }
        }

        viewfinder.onResolutionChanged: console.log("Camera resolution: " + viewfinder.resolution)
    }

    VideoOutput {
        id: videoOutput
        source: camera
        anchors.fill: parent
        focus : visible
        visible: camera.availableCamera && camera.cameraStatus === Camera.ActiveStatus
        autoOrientation: true
        fillMode: VideoOutput.PreserveAspectCrop

        MouseArea{
            anchors.fill: parent
            onDoubleClicked: videoOutput.fillMode = videoOutput.fillMode == VideoOutput.PreserveAspectFit ? VideoOutput.PreserveAspectCrop : VideoOutput.PreserveAspectFit
        }

        filters: [objectsRecognitionFilter]
    }

    // Objects recognizer filter
    ObjectsRecognizer {
        id: objectsRecognitionFilter
        cameraOrientation: camera.orientation
        videoOrientation:  videoOutput.orientation
        contentSize:       Qt.size(videoOutput.width,videoOutput.height)

        minConfidence:   root.minConfidence
        nThreads:        root.nThreads
        showInfTime:     root.showInfTime
        acceleration:    true
    }

    // No camera found info
    Item{
        anchors.centerIn: parent
        width: parent.width
        visible: !camera.availableCamera

        Column{
            width: parent.width

            Text{
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("No camera detected")
            }
        }

    }

    function setActiveLabel(key,value)
    {
        objectsRecognitionFilter.setActiveLabel(key,value)
    }
}
