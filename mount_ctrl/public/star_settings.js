(function() {
    'use strict';
    angular.module('teleCtrl.star_settings')
    .service('star_settings', [function () {
        this.min_magnitude = 5;
        this.star_name_on = true;
        this.star_name_magnitude = 2.5;
        this.height = 960;
        this.width = 960;
        this.constellation_line_on = true;
        this.location = {latitude:33.7946333, longitude: -84.44877199999999};
        this.projection = "polar";
        this.time_on = true;
        this.horizon_on = true;
        this.map_on = false;
        this.cur_star = null;
    }]);
})();