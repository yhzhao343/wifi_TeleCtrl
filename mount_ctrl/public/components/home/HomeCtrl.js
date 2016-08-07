(function() {
    'use strict'
    angular.module('teleCtrl.controller')
    .controller('HomeCtrl', ['$scope', 'star_settings', 'telescope', 'socket', '$interval', function ($scope, star_settings, telescope, socket, $interval) {
        star_settings.map_on = 1;
        $scope.star_settings = star_settings;
        $scope.telescope = {tracking:"2", slew:"4", in_goto:false};
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
        var ask_in_goto;
        $scope.goto_star = function() {
            socket.emit("goto", {telescope:{EQ_Coord:{RA:star_settings.cur_star.ra * 15, DEC:star_settings.cur_star.dec}}});
            ask_in_goto = $interval(function() {
                socket.emit("get_in_goto")
            } , 500)
        }

        socket.on('in_goto_info', function(data) {
            console.log(data)
            if (data == false) {
                $interval.cancel(ask_in_goto);
            };
            $scope.telescope.in_goto = data;
        })
        $scope.set_tracking = function() {
            var msg = {telescope:{tracking:parseInt($scope.telescope.tracking)}}
            socket.emit("set_tracking", msg);
        }
    }])
})();