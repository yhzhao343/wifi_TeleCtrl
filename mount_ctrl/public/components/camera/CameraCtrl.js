(function() {
    'use strict'
    angular.module('teleCtrl.controller')
    .controller('CameraCtrl', ['$scope', 'cam', 'socket', '$interval', '$window', function ($scope, cam, socket, $interval, $window) {
        $scope.counter = 0;
        var counterInterval = undefined;
        function incrementCounter() {
            $scope.counter++;
        }
        $scope.startCounter = function startCounter() {
            counterInterval = $interval(incrementCounter, 1000);
        }
        $scope.clearCounter = function clearCounter() {
            $interval.cancel(counterInterval);
            $scope.counter = 0;
        }
        $scope.shutterPressed = false;
        $scope.info = {debug:""};
        $scope.cam = cam;
        $scope.autoDetect = cam.autoDetect;
        socket.on('debug', function(message) {
            $scope.info.debug = message
        })
        $scope.fileName="";
        $scope.custom_gphoto2_cmd = ""
        $scope.download = function() {
            $window.open('http://192.168.0.15:8080/image/' + $scope.fileName);
            $scope.custom_gphoto2_cmd = ""
        }
        $scope.send_custom_gphoto2_cmd = function() {
            socket.emit('command', {command:$scope.custom_gphoto2_cmd})
            $scope.custom_gphoto2_cmd = ""
        }
    }])
})();