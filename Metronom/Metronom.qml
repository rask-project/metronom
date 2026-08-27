import QtQuick
import QtQuick.VectorImage
import QtQuick.VectorImage.Helpers
import QtQuick.Shapes
import QtQuick.Effects

Item {
    id: _qt_node0

    required property Metronome metronome

    property AnimationsInfo animations: AnimationsInfo {}
    transform: [
        Scale {
            xScale: width / 481.6
            yScale: height / 481.6
        }
    ]

    component AnimationsInfo: QtObject {
        property bool paused: false
        property int loops: 1
        signal restart
    }

    implicitWidth: 124
    implicitHeight: 124

    objectName: "Layer_1"
    transformOrigin: Item.TopLeft

    Shape {
        id: _qt_node1
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            id: _qt_node1_fill_stroke
            strokeColor: "transparent"
            fillColor: "#ff2c3331"
            fillRule: ShapePath.WindingFill
            pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
            PathSvg {
                path: "M 375.2 456.8 Q 375.2 467 367.9 474.3 Q 360.6 481.6 350.4 481.6 L 105.6 481.6 Q 95.4 481.6 88.1 474.3 Q 80.8 467 80.8 456.8 Q 80.8 446.6 88.1 439.3 Q 95.4 432 105.6 432 L 349.6 432 Q 360.338 432 367.8 439.3 Q 375.2 446.539 375.2 456.8 "
            }
        }
    }

    Shape {
        id: _qt_node2
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            id: _qt_node2_fill_stroke
            strokeColor: "transparent"
            fillColor: "#ffb58539"
            fillRule: ShapePath.WindingFill
            pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
            PathSvg {
                path: "M 397.6 407.2 L 324.8 19.2 Q 322.985 10.7309 316.4 5.4 Q 309.729 0 300.8 0 L 154.4 0 Q 145.929 0 139.1 5.4 Q 132.186 10.8672 130.4 19.2 L 57.6 410.4 Q 57.0984 412.657 55.1727 417.123 Q 53.1133 421.898 52.7689 424.133 Q 52.1986 427.832 54.4 430.4 Q 61.9429 439.2 73.6 439.2 L 383.2 439.2 Q 392.569 439.2 396.3 431.9 Q 399.2 426.226 399.2 414.4 Q 399.2 412.8 398.7 410.8 Q 398.4 409.6 397.6 407.2 "
            }
        }
    }

    Shape {
        id: _qt_node3
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            id: _qt_node3_fill_stroke
            strokeColor: "transparent"
            fillColor: "#ff997330"
            fillRule: ShapePath.WindingFill
            pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
            PathSvg {
                path: "M 405.6 422.4 L 324.8 34.4 Q 322.985 25.9309 316.4 20.6 Q 309.729 15.2 300.8 15.2 L 154.4 15.2 Q 145.929 15.2 139.1 20.6 Q 132.186 26.0672 130.4 34.4 L 49.6 425.6 Q 47.0588 437.035 54.4 445.6 Q 61.9429 454.4 73.6 454.4 L 383.2 454.4 Q 393.4 454.4 400.7 447.1 Q 408 439.8 408 429.6 L 405.6 422.4 "
            }
        }
    }

    Shape {
        id: _qt_node4
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            id: _qt_node4_fill_stroke
            strokeColor: "transparent"
            fillColor: "#ffba8b3e"
            fillRule: ShapePath.WindingFill
            pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
            PathSvg {
                path: "M 405.6 422.4 L 324.8 34.4 Q 322.985 25.9309 316.4 20.6 Q 309.729 15.2 300.8 15.2 L 154.4 15.2 Q 145.929 15.2 139.1 20.6 Q 132.186 26.0672 130.4 34.4 L 49.6 425.6 Q 47.0588 437.035 54.4 445.6 Q 61.9429 454.4 73.6 454.4 L 383.2 454.4 Q 393.4 454.4 400.7 447.1 Q 408 439.8 408 429.6 L 405.6 422.4 "
            }
        }
    }

    Shape {
        id: _qt_node5
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            id: _qt_node5_fill_stroke
            strokeColor: "transparent"
            fillColor: "#ff997330"
            fillRule: ShapePath.WindingFill
            pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
            PathSvg {
                path: "M 405.6 422.4 L 324.8 34.4 Q 322.985 25.9309 316.4 20.6 Q 309.729 15.2 300.8 15.2 L 154.4 15.2 Q 145.929 15.2 139.1 20.6 Q 132.186 26.0672 130.4 34.4 L 103.2 425.6 Q 100.659 437.035 108 445.6 Q 115.543 454.4 127.2 454.4 L 382.4 454.4 Q 392.6 454.4 399.9 447.1 Q 407.2 439.8 407.2 429.6 Q 407.2 428.01 406.707 426.028 Q 406.406 424.819 405.619 422.456 L 405.6 422.4 "
            }
        }
    }

    Shape {
        id: _qt_node6
        preferredRendererType: Shape.CurveRenderer
        transformOrigin: Item.TopLeft

        ShapePath {
            id: _qt_node7_fill_stroke
            strokeColor: "transparent"
            fillColor: "#ff8c6425"
            fillRule: ShapePath.WindingFill
            pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
            PathSvg {
                path: "M 382.4 454.4 Q 392.6 454.4 399.9 447.1 Q 407.2 439.8 407.2 429.6 Q 407.2 427.933 406.7 425.6 Q 406.28 423.64 405.6 421.6 L 324.8 34.4 Q 322.985 25.9309 316.4 20.6 Q 309.729 15.2 300.8 15.2 L 154.4 15.2 Q 145.929 15.2 139.1 20.6 Q 132.186 26.0672 130.4 34.4 L 382.4 454.4 "
            }
        }

        ShapePath {
            id: _qt_node8_fill_stroke
            strokeColor: "transparent"
            fillColor: "#ff8c6425"
            fillRule: ShapePath.WindingFill
            pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
            PathSvg {
                path: "M 60.8 371.2 L 395.2 371.2 L 388.8 338.4 L 67.2 338.4 L 60.8 371.2 "
            }
        }
    }

    Shape {
        id: _qt_node9
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            id: _qt_node9_fill_stroke
            strokeColor: "transparent"
            fillColor: "#ff9faaa8"
            fillRule: ShapePath.WindingFill
            pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
            PathSvg {
                path: "M 64 354.4 L 392 354.4 L 325.6 36.8 L 130.4 36.8 L 64 354.4 "
            }
        }
    }

    Shape {
        id: _qt_node10
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            id: _qt_node10_fill_stroke
            strokeColor: "transparent"
            fillColor: "#ff84918f"
            fillRule: ShapePath.WindingFill
            pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
            PathSvg {
                path: "M 109.6 354.4 L 392 354.4 L 325.6 36.8 L 130.4 36.8 L 109.6 354.4 "
            }
        }
    }

    Shape {
        id: _qt_node11
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            id: _qt_node11_fill_stroke
            strokeColor: "transparent"
            fillColor: "#ff6b7573"
            fillRule: ShapePath.WindingFill
            pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
            PathSvg {
                path: "M 392 354.4 L 325.6 36.8 L 130.4 36.8 L 392 354.4 "
            }
        }
    }

    Shape {
        id: _qt_node12
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            id: _qt_node12_fill_stroke
            strokeColor: "transparent"
            fillColor: "#ff505957"
            fillRule: ShapePath.WindingFill
            pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
            PathSvg {
                path: "M 234.4 354.4 L 364.8 224.8 L 360 201.6 L 207.2 354.4 L 234.4 354.4 "
            }
        }
    }

    Shape {
        id: _qt_node13
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            id: _qt_node13_fill_stroke
            strokeColor: "transparent"
            fillColor: "#ff2c3331"
            fillRule: ShapePath.WindingFill
            pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
            PathSvg {
                path: "M 274.4 354.4 L 274.4 95.2 Q 274.4 82.6407 264.9 76.2 Q 256.345 70.4 241.6 70.4 L 214.4 70.4 Q 199.862 70.4 191.4 75.9 Q 181.6 82.27 181.6 95.2 L 181.6 354.4 L 274.4 354.4 "
            }
        }
    }

    Shape {
        id: _qt_node14
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            id: _qt_node14_fill_stroke
            strokeColor: "transparent"
            fillColor: "#ff080a09"
            fillRule: ShapePath.WindingFill
            pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
            PathSvg {
                path: "M 242.4 70.4 L 215.2 70.4 Q 200.662 70.4 192.2 75.9 Q 182.4 82.27 182.4 95.2 L 182.4 142.4 Q 182.4 129.841 191.9 123.4 Q 200.455 117.6 215.2 117.6 L 242.4 117.6 Q 256.938 117.6 265.4 123.1 Q 275.2 129.47 275.2 142.4 L 275.2 95.2 Q 274.122 70.4 242.4 70.4 "
            }
        }
    }

    Item {
        id: pendulum
        transform: [
            Rotation {
                id: swing
                origin: Qt.point(197.6, 355.2)
                axis: Qt.vector3d(0, 0, 1)
                angle: -45.8
            }
        ]

        Shape {
            id: _qt_node15
            transformOrigin: Item.TopLeft
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                id: _qt_node15_fill_stroke
                strokeColor: "transparent"
                fillColor: "#ffefd862"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg {
                    path: "M 429.6 122.4 Q 426.6 119.4 422.8 119.4 Q 419 119.4 416 122.4 L 184 355.2 L 211.2 355.2 L 429.6 136.8 Q 432.6 133.2 432.6 129.3 Q 432.6 125.4 429.6 122.4 "
                }
            }
        }

        Shape {
            id: _qt_node16
            transformOrigin: Item.TopLeft
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                id: _qt_node16_fill_stroke
                strokeColor: "transparent"
                fillColor: "#ffe2c444"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg {
                    path: "M 184 355.2 L 211.2 355.2 L 429.6 136.8 Q 432.6 133.8 432.6 130 Q 432.6 126.2 429.6 123.2 L 184 355.2 "
                }
            }
        }

        Shape {
            id: _qt_node17
            transformOrigin: Item.TopLeft
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                id: _qt_node17_fill_stroke
                strokeColor: "transparent"
                fillColor: "#ffffac1a"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg {
                    path: "M 388 192.8 Q 388 205.061 379.33 213.73 Q 370.661 222.4 358.4 222.4 Q 346.139 222.4 337.47 213.73 Q 328.8 205.061 328.8 192.8 Q 328.8 180.539 337.47 171.87 Q 346.139 163.2 358.4 163.2 Q 370.661 163.2 379.33 171.87 Q 388 180.539 388 192.8 "
                }
            }
        }

        Shape {
            id: _qt_node18
            transformOrigin: Item.TopLeft
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                id: _qt_node18_fill_stroke
                strokeColor: "transparent"
                fillColor: "#ffff7a17"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg {
                    path: "M 387.2 192.8 Q 387.2 204.94 378.7 213.6 Q 370.063 222.4 357.6 222.4 Q 345.46 222.4 336.8 213.9 Q 328 205.263 328 192.8 L 387.2 192.8 "
                }
            }
        }
    }

    NumberAnimation {
        id: swingLeft
        target: swing
        property: "angle"
        to: -91.6
        duration: 60000 / metronome.bpm / Math.pow(2, metronome.noteValue)
        easing.type: Easing.InOutSine
    }

    NumberAnimation {
        id: swingRight
        target: swing
        property: "angle"
        to: 0
        duration: 60000 / metronome.bpm / Math.pow(2, metronome.noteValue)
        easing.type: Easing.InOutSine
    }

    NumberAnimation {
        id: settle
        target: swing
        property: "angle"
        to: -45.8
        duration: 400
        easing.type: Easing.OutCubic
    }

    Connections {
        target: metronome

        function onRunningChanged() {
            if (metronome.running) {
                settle.stop();
                swing.angle = 0;
                return;
            }

            swingLeft.stop();
            swingRight.stop();
            settle.start();
        }

        function onCurrentBeatChanged() {
            if (!metronome.running) {
                return;
            }

            if (metronome.currentBeat % 2 === 0) {
                swingLeft.start();
                return;
            }

            swingRight.start();
        }
    }
}
