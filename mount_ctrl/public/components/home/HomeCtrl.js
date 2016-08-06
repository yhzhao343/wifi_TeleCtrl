(function() {
    'use strict'
    angular.module('teleCtrl.controller')
    .controller('HomeCtrl', ['$scope', 'star_settings', 'telescope', 'socket', function ($scope, star_settings, telescope, socket) {
        star_settings.map_on = 1;
        $scope.star_settings = star_settings;
        $scope.telescope = {tracking:"2", slew:"4"};
        $scope.focuser = {mode:"1", interval:25750};
        $scope.focuser_plus = function() {
            socket.emit('focuser_mode', parseInt($scope.focuser.mode));
        }
        $scope.focuser_minus = function() {
            socket.emit('focuser_mode', -parseInt($scope.focuser.mode));
        }
        $scope.focuser_stop = function() {
            socket.emit('focuser_mode', 0);
        }

        $scope.set_interval = function() {
            var msg = {focuser:{mode:parseInt($scope.focuser.mode), interval:parseInt($scope.focuser.interval)}};
            socket.emit('focuser', msg);
        }

        $scope.set_slew = function(axis, dir) {
            var string = axis + dir + $scope.telescope.slew;
            socket.emit('set_slew', {telescope:{slew:string}});
        }

        $scope.goto_star = function() {
            var ra = (Math.round(star_settings.cur_star.ra/360*16777216)).toString(16);
            var dec = (Math.round(star_settings.cur_star.dec/360*16777216)).toString(16);
            ra = "000000".substring(0, 6-ra.length) + ra + "00"
            dec = "000000".substring(0, 6-dec.length) + dec + "00"
            var string = "r" + ra.toUpperCase() + "," + dec.toUpperCase();
            socket.emit("goto", {telescope:{goto:string}});
            console.log(string);
        }
        $scope.set_tracking = function() {
            var msg = {telescope:{tracking:parseInt($scope.telescope.tracking)}}
            socket.emit("set_tracking", msg);
        }
    }])
})();