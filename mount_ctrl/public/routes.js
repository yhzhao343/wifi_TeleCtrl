(function() {
    'use strict';
    angular.module('teleCtrl.routes', ['ngRoute'])
    .config(['$routeProvider', function($routeProvider) {
        $routeProvider
        .when('/home', {
            templateUrl:'components/home/home.html',
            controller:'HomeCtrl'
        })
        .when('/camera', {
            templateUrl:'components/camera/camera.html',
            controller: 'CameraCtrl'
        })
        .otherwise({ redirectTo: '/home' });
    }])
})();