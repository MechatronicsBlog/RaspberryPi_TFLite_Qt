import QtQuick 2.9
import QtQuick.Controls 2.2

Page {
    id: root
    title: qsTr("Settings")

    property double minConfidence
    property int    nThreads
    property bool   showInfTime
    property var    tfObjects: []
    property string resolution
    property var    resolutions: []
    property bool   loadingRes

    readonly property int leftMargin: 100
    readonly property int rightMargin: 100

    signal objectChanged(string label, bool checked)
    signal resolutionUpdated()

    onResolutionsChanged: {
        console.log("Camera resolutions: " + resolutions)
        loadingRes = true
        sbResolution.to    = resolutions.length - 1
        sbResolution.items = resolutions
        sbResolution.value = -1
        loadingRes         = false
        sbResolution.value = sbResolution.valueFromText(resolution)
    }

    // TabBar
    footer: TabBar {
                  id: tabBar
                  currentIndex: swipeView.currentIndex

                  TabButton {
                      text: qsTr("Neural Network && Camera")
                  }

                  TabButton {
                    text: qsTr("Screen info")
                  }

                  TabButton {
                      text: qsTr("Hardware info && Close app")
                  }
                }



    // tab contents
       SwipeView {
         id: swipeView
         anchors.fill: parent
         clip: true
         currentIndex: tabBar.currentIndex

         Flickable {
             contentHeight: column.height
             contentWidth:  column.width

             flickableDirection: Flickable.VerticalFlick

             Column{
                 id:    column
                 width: root.width

                 Item{
                     height: 20
                     width: 1
                 }

                 Text{
                     anchors.leftMargin:  leftMargin
                     anchors.rightMargin: rightMargin
                     anchors.horizontalCenter: parent.horizontalCenter
                     horizontalAlignment: Text.AlignHCenter
                     width: parent.width
                     wrapMode: Text.WordWrap
                     elide: Text.ElideRight
                     text: qsTr("Minimum confidence")
                 }

                 Item{
                     width: 1
                     height: 10
                 }

                 Slider{
                     id: slider
                     anchors.horizontalCenter: parent.horizontalCenter
                     width: parent.width - (leftMargin+rightMargin)
                     from:  0
                     to:    1
                     value: minConfidence
                     live:  true

                     onValueChanged: minConfidence = value
                 }

                 Text {
                     anchors.leftMargin:  leftMargin
                     anchors.rightMargin: rightMargin
                     anchors.horizontalCenter: parent.horizontalCenter
                     horizontalAlignment: Text.AlignHCenter
                     width: parent.width
                     wrapMode: Text.WordWrap
                     elide: Text.ElideRight
                     text: Math.round(slider.position * 100) + " %"
                 }

                 Item{
                     width: 1
                     height: 10
                 }

                 Row {
                     width: parent.width
                     spacing: 2

                     Item{
                         height: 1
                         width: (parent.width - parent.spacing - tThread.width)*0.5
                     }

                     Text{
                         id: tThread
                         anchors.leftMargin: 30
                         anchors.verticalCenter: parent.verticalCenter
                         verticalAlignment: Text.AlignVCenter
                         wrapMode: Text.WordWrap
                         elide: Text.ElideRight
                         text: qsTr("Number of threads")
                     }
                 }

                 Slider{
                     id: sThreads
                     anchors.horizontalCenter: parent.horizontalCenter
                     width: parent.width - (leftMargin+rightMargin)
                     from:  1
                     to:    auxUtils.numberThreads()
                     enabled: to>1
                     live:  true
                     snapMode: Slider.SnapAlways
                     stepSize: 1
                     value: nThreads

                     onValueChanged: nThreads = value
                 }

                 Text {
                     anchors.leftMargin:  leftMargin
                     anchors.rightMargin: rightMargin
                     anchors.horizontalCenter: parent.horizontalCenter
                     horizontalAlignment: Text.AlignHCenter
                     width: parent.width
                     wrapMode: Text.WordWrap
                     elide: Text.ElideRight
                     text: sThreads.value + " " + (sThreads.value>1 ? qsTr("threads") : qsTr("thread"))
                 }

                 Item{
                     width: 1
                     height: 20
                 }

                 Text{
                     anchors.leftMargin:  leftMargin
                     anchors.rightMargin: rightMargin
                     anchors.horizontalCenter: parent.horizontalCenter
                     horizontalAlignment: Text.AlignHCenter
                     width: parent.width
                     wrapMode: Text.WordWrap
                     elide: Text.ElideRight
                     text: qsTr("Camera resolution")
                 }

                 Item{
                     width: 1
                     height: 10
                 }

                 SpinBox {
                     id: sbResolution
                     anchors.horizontalCenter: parent.horizontalCenter
                     from:     0
                     editable: false
                     property var items

                     textFromValue: function(value) {
                         return items[value];
                     }

                     valueFromText: function(text) {
                         for (var i = 0; i < items.length; i++)
                         {
                             if (items[i].toLowerCase() === text.toLowerCase())
                                 return i
                         }
                         return value
                     }

                     onValueChanged:
                         if (value>=0 && !loadingRes)
                         {
                             resolution = textFromValue(value)
                             console.log("Resolution to save: " + resolution)
                             resolutionUpdated()
                         }
                 }

                 Item{
                     height: 30
                     width: 1
                 }
             }
         }

         Flickable {
             id: flickInfo
             contentHeight: column3.height
             contentWidth:  column3.width

             flickableDirection: Flickable.VerticalFlick

             Column{
                 id:    column3
                 width: root.width

                 Item{
                     height: 20
                     width: 1
                 }

                 Text{
                     anchors.leftMargin:  leftMargin
                     anchors.rightMargin: rightMargin
                     anchors.horizontalCenter: parent.horizontalCenter
                     horizontalAlignment: Text.AlignHCenter
                     width: parent.width
                     wrapMode: Text.WordWrap
                     elide: Text.ElideRight
                     text: qsTr("General information")
                 }

                 Item{
                     width: 1
                     height: 10
                 }

                 Row{
                     width: parent.width - (leftMargin+rightMargin)
                     anchors.horizontalCenter: parent.horizontalCenter
                     spacing: width - tShowInfTime.width - sShowInfTime.width

                     Text {
                         id: tShowInfTime
                         text: qsTr("Show inference time")
                         anchors.verticalCenter: parent.verticalCenter
                         verticalAlignment: Text.AlignVCenter
                     }

                     Switch{
                         anchors.verticalCenter: parent.verticalCenter
                         id: sShowInfTime
                         checked: showInfTime

                         onToggled: showInfTime = checked
                     }
                 }

                 Item{
                     height: 20
                     width: 1
                 }

                 Text{
                     anchors.leftMargin:  leftMargin
                     anchors.rightMargin: rightMargin
                     anchors.horizontalCenter: parent.horizontalCenter
                     horizontalAlignment: Text.AlignHCenter
                     width: parent.width
                     wrapMode: Text.WordWrap
                     elide: Text.ElideRight
                     text: qsTr("Detected objects")
                 }

                 Item{
                     width: 1
                     height: 10
                 }

                 Repeater{
                     model: auxUtils.getLabels()

                     Column{
                         width: root.width

                         Row{
                             width: parent.width - (leftMargin+rightMargin)
                             anchors.horizontalCenter: parent.horizontalCenter
                             spacing: width - rObject.width - sObject.width


                             Row {
                                 id: rObject
                                 spacing: 2
                                 width: tObject.width + spacing

                                 Text {
                                     id: tObject
                                     text: modelData.replace(/\b\w/g, function(l){ return l.toUpperCase() })
                                     anchors.verticalCenter: parent.verticalCenter
                                     verticalAlignment: Text.AlignVCenter
                                     wrapMode: Text.NoWrap
                                     elide: Text.ElideRight
                                 }
                             }

                             Switch{
                                 anchors.verticalCenter: parent.verticalCenter
                                 id: sObject
                                 checked: tfObjects[modelData]
                                 onToggled: objectChanged(modelData,checked)
                             }

                         }

                         Item{
                             width: 1
                             height: 10
                         }
                     }

                 }
             }
         }

         Flickable {
             contentHeight: column2.height
             contentWidth:  column2.width

             flickableDirection: Flickable.VerticalFlick

             Column{
                 id:    column2
                 width: root.width

                 Item{
                     height: 20
                     width: 1
                 }

                 Text{
                     anchors.leftMargin:  leftMargin
                     anchors.rightMargin: rightMargin
                     anchors.horizontalCenter: parent.horizontalCenter
                     horizontalAlignment: Text.AlignHCenter
                     width: parent.width
                     wrapMode: Text.WordWrap
                     elide: Text.ElideRight
                     height: 30
                     text: qsTr("Hardware info")
                 }

                 Item{
                     width: 1
                     height: 10
                 }

                 Column{
                     width: root.width

                     Repeater{
                         model: auxUtils.networkInterfaces()

                         Text{
                              anchors.leftMargin:  leftMargin
                              anchors.rightMargin: rightMargin
                              horizontalAlignment: Text.AlignHCenter
                              width: parent.width
                              height: 30
                              wrapMode: Text.WordWrap
                              elide: Text.ElideRight
                              text: modelData
                         }
                     }
                 }

                 Item{
                     width: 1
                     height: 30
                 }

                 Button{
                     id: bClose
                     anchors.horizontalCenter: parent.horizontalCenter
                     text: qsTr("Close app")
                     onClicked: dialog.open()

                     contentItem: Text {
                            text: bClose.text
                            font: bClose.font
                            opacity: enabled ? 1.0 : 0.3
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }

                     background: Rectangle {
                         implicitWidth: 100
                         implicitHeight: 40
                         color: bClose.down ? "#ff0000" : "#aa0000"
                         border.color: "#7e181a"
                         border.width: 1
                         radius: 0
                     }
                 }
             }
         }

       }

       Rectangle{
           id: backPageIndicator
           anchors.bottom: parent.bottom
           height: 20
           width: parent.width
       }

       PageIndicator {
           id:               pageIndicator
           count:            swipeView.count
           currentIndex:     swipeView.currentIndex
           anchors.centerIn: backPageIndicator

           delegate: Rectangle{
               implicitWidth:  10
               implicitHeight: 10
               radius: width
           }
       }

       Dialog {
           id: dialog
           x: 0.5*(parent.width - width)
           y: 0.5*(parent.height - height)
           title: "Close app"
           standardButtons: Dialog.Ok | Dialog.Cancel
           modal: true

           Label{
               text: qsTr("Do you really want to close this app?")
           }

           onAccepted: Qt.callLater(Qt.quit)
           onRejected: dialog.close()
       }
}
