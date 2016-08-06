(function() {
    'use strict';
    angular.module('teleCtrl.star_calc')
    .factory('star_calc', ['star_database', 'star_settings',function (star_database, star_settings) {
        var month_start_day = [0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334];
        var year_day_since_J2000 = {'2016':5842.5, '2017':6208.5, '2018': 6573.5, '2019': 6938.5, '2020':7303.5, '2021':7669.5}
        var deg2rad = function(deg) {
            return deg * Math.PI / 180;
        }
        var rad2deg = function(rad) {
            return rad * 180 / Math.PI;
        }
        var stereo = function(star) {
            var el = star.ALT
            var az = star.AZ;
            if (el < 0 || isNaN(el) || isNaN(az)) {
                return;
            }
            var w = star_settings.width;
            var h = star_settings.height;
            var f = 0.42;
            var sinel1 = 0;
            var cosel1 = 1;
            var cosaz = Math.cos((az-Math.PI));
            var sinaz = Math.sin((az-Math.PI));
            var sinel = Math.sin(el);
            var cosel = Math.cos(el);
            var k = 2/(1+sinel1*sinel+cosel1*cosel*cosaz);
            star.x = w/2+f*k*h*cosel*sinaz;
            star.y = (h-f*k*h*(cosel1*sinel-sinel1*cosel*cosaz));
            return star;
        }
        var polar = function(star) {
            var az = star.AZ;
            var el = star.ALT;
            if (el < 0 || isNaN(az) || isNaN(el)) {
                return;
            }
            var w = star_settings.width;
            var h = star_settings.height;
            var radius = h/2;
            var r = radius*((Math.PI/2)-el)/(Math.PI/2);
            star.x = (w/2-r*Math.sin(az));
            star.y = (radius-r*Math.cos(az))
            return star;
        }
        return {
            radec2azel: function() {
                var date = new Date();
                var location = star_settings.location;
                var UTCDate = date.getUTCDate();
                var UTCMonth = date.getUTCMonth();
                var UTCMinutes = date.getUTCMinutes();
                var UTCHours = date.getUTCHours();
                var UTCSeconds = date.getUTCSeconds();
                var UTCYear = date.getUTCFullYear();
                var dayNum = month_start_day[UTCMonth] + UTCDate + UTCHours/24 + UTCMinutes/60 + UTCSeconds/3600;
                var year = UTCYear.toString();
                if(((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0) && UTCMonth > 2) {
                    dayNum++;
                }
                var dayFromJ2000 = year_day_since_J2000[UTCYear] + dayNum;
                var LST = 100.46 + 0.985647 * dayFromJ2000 + location.longitude + 15 * (UTCHours + UTCMinutes/60);
                LST = LST % 360;
                star_database.stars.forEach(function(star) {
                    star.HA = LST - star.ra * 15;
                    var x = Math.cos(deg2rad(star.HA)) * Math.cos(deg2rad(star.dec));
                    var y = Math.sin(deg2rad(star.HA)) * Math.cos(deg2rad(star.dec));
                    var z = Math.sin(deg2rad(star.dec));
                    var xhor = x * Math.cos(deg2rad(90 - location.latitude)) - z * Math.sin(deg2rad(90 - location.latitude));
                    var yhor = y;
                    var zhor = x * Math.sin(deg2rad(90 - location.latitude)) + z * Math.cos(deg2rad(90 - location.latitude));
                    star.AZ = Math.atan2(yhor, xhor) + Math.PI;
                    star.ALT = Math.asin(zhor);
                })
            },
            project:function() {
                var visible_stars = [];
                for (var i = 0; i < star_database.stars.length; i++) {
                    var star = star_database.stars[i]
                    var processed_star;
                    switch(star_settings.projection) {
                        case "polar" :
                            processed_star = polar(star);
                            break;
                        case "stereo" :
                            processed_star = stereo(star);
                            break;
                        default:
                            processed_star = polar(star);
                    }
                }
            }
        };
    }])

})();