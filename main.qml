import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.LocalStorage 2.0
import "storage.js" as Settings

ApplicationWindow {
    id: window
    visible: true
    title: qsTr("TensorFlow Lite & Qt")

    readonly property string tMinConfidence: 'minConfidence'
    readonly property string tNThreads:      'nThreads'
    readonly property string tShowInfTime:   'showInfTime'
    readonly property string tResolution:    'resolution'

    // Default values
    readonly property double defMinConfidence:   0.7
    readonly property bool   defShowInfTime:     false
    readonly property int    defNumThreads:      1
    readonly property bool   defTFObject:        true
    readonly property string defResolution:      "640x480"

    property double minConfidence:   Settings.get(tMinConfidence,defMinConfidence)
    property int    nThreads:        Settings.get(tNThreads,defNumThreads)
    property bool   showInfTime:     Settings.get(tShowInfTime,defShowInfTime) == 0 ? false : true
    property string resolution:      Settings.get(tResolution,defResolution)
    property var    tfObjects:       []

    header: ToolBar {
        contentHeight: toolButton.implicitHeight

        ToolButton {
            id: toolButton
            text: stackView.depth > 1 ? "\u25C0" : "\u2630"
            font.pixelSize: Qt.application.font.pixelSize * 1.6
            onClicked: {
                if (stackView.depth > 1) {
                    stackView.pop()
                } else {
                    drawer.open()
                }
            }
        }

        Label {
            text: stackView.currentItem.title
            anchors.centerIn: parent
        }
    }

    Drawer {
        id: drawer
        width: window.width * 0.3
        height: window.height

        Column {
            anchors.fill: parent

            ItemDelegate {
                text: qsTr("Settings")
                width: parent.width
                onClicked: {
                    stackView.push(configuration)
                    drawer.close()
                }
            }
        }
    }

    StackView {
        id: stackView
        initialItem: home
        anchors.fill: parent
    }

    Home{
        id: home
        visible: false
        minConfidence:   window.minConfidence
        nThreads:        window.nThreads
        showInfTime:     window.showInfTime
        resolution:      window.resolution
    }

    Configuration{
        id: configuration
        visible: false

        minConfidence:   window.minConfidence
        nThreads:        window.nThreads
        showInfTime:     window.showInfTime
        resolution:      window.resolution
        resolutions:     home.resolutions

        onMinConfidenceChanged:   {Settings.set(tMinConfidence,minConfidence); window.minConfidence = minConfidence }
        onNThreadsChanged:        {Settings.set(tNThreads,nThreads); window.nThreads = nThreads }
        onShowInfTimeChanged:     {Settings.set(tShowInfTime,showInfTime); window.showInfTime = showInfTime }
        onResolutionUpdated:      {Settings.set(tResolution,resolution); window.resolution = resolution}

        onObjectChanged: {
            tfObjects[label] = checked
            Settings.set(label,checked)
            setActiveLabel(label,checked)
        }
    }

    Component.onCompleted: init()

    function init()
    {
        console.log("Initialization")

        var labels = auxUtils.getLabels()

        tfObjects = []
        for(var i=0;i<labels.length;i++)
        {
            var value = Settings.get(labels[i],defTFObject) == 0 ? false : true
            tfObjects[labels[i]] = value
        }
        home.tfObjects = Qt.binding(function() { return tfObjects })
        configuration.tfObjects = Qt.binding(function() { return tfObjects })
        setAllLabels()
    }

    function setActiveLabel(key,value)
    {
        home.setActiveLabel(key,value)
    }

    function setAllLabels()
    {
        var labels = auxUtils.getLabels()

        for(var i=0;i<labels.length;i++)
            setActiveLabel(labels[i],tfObjects[labels[i]])
    }
}
