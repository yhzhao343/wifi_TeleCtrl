(function() {
    'use strict'
    angular.module('teleCtrl.controller')
    .controller('CameraCtrl', ['$scope', 'cam', 'socket', '$interval', function ($scope, cam, socket, $interval) {
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
    }])
})();