(function() {
    'use strict'
    angular.module('teleCtrl.directives')
      .directive('starMap', ['d3Service', 'star_calc', 'star_settings', 'star_database', function(d3Service, star_calc, star_settings, star_database) {
          var controller = ['$scope', '$q', '$interval', function($scope, $q, $interval) {
            var draw_star_map = function() {
                var deferred = $q.defer();
                var width = star_settings.width;
                var height = star_settings.height;
                var randomX = d3.random.normal(width / 2, 80);
                var randomY = d3.random.normal(height / 2, 80);
                star_calc.radec2azel();
                star_calc.project();


                var x = d3.scale.linear()
                    .domain([0, width])
                    .range([0, width]);

                var y = d3.scale.linear()
                    .domain([0, height])
                    .range([height, 0]);

                var svg = d3.select("#star_map_container")
                    .append("svg")
                    .attr("id","star_map")
                    .attr("width", width)
                    .attr("height", height)
                  .append("g");

                svg.append("rect")
                    .attr("class", "overlay")
                    .attr("width", width)
                    .attr("height", height);

                if (star_settings.time_on) {
                    var time_text = svg.append("text")
                    .attr("x", 15)
                    .attr("y", 15)
                    .text(new Date().toString())
                }

                var bright_data = star_database.stars.slice(0, (function() {
                    for (var i = 0; i < star_database.stars.length; i++) {
                        if(star_database.stars[i].mag > star_settings.min_magnitude) {
                            return i;
                        }
                    }
                })());

                var data = bright_data.filter(function(star) {if (star.ALT > 0) {return true} else {return false}});

                if (star_settings.cur_star) {
                    draw_cur_star();
                }

                if (star_settings.constellation_line_on) {
                    for (var i = 0; i < star_database.constellation_line.length; i++) {
                        var cur_line = star_database.constellation_line[i];
                        var a = cur_line.a;
                        var b = cur_line.b;
                        if (!(a == -1 || b == -1)) {
                            var star_a = star_database.stars[a];
                            var star_b = star_database.stars[b];
                            if (star_a.ALT > 0 && star_b.ALT > 0) {
                                svg.append('line')
                                .attr('x1', star_a.x)
                                .attr('y1', star_a.y)
                                .attr('x2', star_b.x)
                                .attr('y2', star_b.y)
                                .attr('stroke-width', 0.5)
                                .attr('stroke', 'black')
                            }
                        }
                    }
                }

                for (var i = 0; i < data.length; i++) {
                    var d = data[i];
                    svg.append('circle')
                    .attr("cx", d.x || 0)
                    .attr("cy", d.y || 0)
                    .attr("r", (function() {
                        var mag = d.mag;
                        if (mag > star_settings.min_magnitude || !d.x || !d.y) {
                            return 0
                        } else {
                            return 1.2*Math.max(3-d.mag/2.1, 0.5) + 0.5
                        }
                    })())
                    .on('click', (click_gen(d)))
                    //.attr('ng-click', 'click_gen(d)()')
                }

                function click_gen(star) {
                    return function() {
                        star_settings.cur_star = star;
                        draw_cur_star();
                    }
                }

                $scope.click_gen = click_gen;

                if (star_settings.star_name_on) {
                    var star_name = svg.selectAll("text")
                        .data(data)
                        .enter().append("text")
                        .attr("x", function(d){return d.x + 5 || 0})
                        .attr("y", function(d){return d.y + 5 || 0})
                        .text(function(d) {if (d.mag < star_settings.star_name_magnitude) {return d.proper}});
                }

                if (star_settings.horizon_on) {
                    var horizon_r = width < height ? width/2 : height/2;
                    var horizon = svg.append("circle")
                    .attr("cx", width/2)
                    .attr("cy", height/2)
                    .attr("r", horizon_r)
                    .attr("fill", "none")
                    .attr("stroke-width", 1)
                    .attr("stroke", "blue")
                };

                function draw_cur_star() {
                    if (star_settings.cur_star) {
                        d3.select("#current_selected_star").remove();
                        d3.select("#current_selected_star_x").remove();
                        d3.select("#current_selected_star_y").remove();
                    };
                    svg.append("circle")
                    .attr("id" , "current_selected_star")
                    .attr("cx", star_settings.cur_star.x)
                    .attr("cy", star_settings.cur_star.y)
                    .attr("r", function() {
                        return 1.2*Math.max(3-star_settings.cur_star.mag/2.1, 0.5) + 4.5
                    })
                    .attr("fill", "none")
                    .attr("stroke", "red")
                    .attr("stroke-width", 2);
                    svg.append("line")
                    .attr('id', 'current_selected_star_x')
                    .attr('x1', star_settings.cur_star.x-10)
                    .attr('y1', star_settings.cur_star.y)
                    .attr('x2', star_settings.cur_star.x+10)
                    .attr('y2', star_settings.cur_star.y)
                    .attr('stroke', 'red')
                    .attr('stroke-width', 1)
                    svg.append("line")
                    .attr('id', 'current_selected_star_y')
                    .attr('x1', star_settings.cur_star.x)
                    .attr('y1', star_settings.cur_star.y-10)
                    .attr('x2', star_settings.cur_star.x)
                    .attr('y2', star_settings.cur_star.y+10)
                    .attr('stroke', 'red')
                    .attr('stroke-width', 1)
                }

                deferred.resolve();
                return deferred.promise;
            }
            var interval;
            var promise = d3Service.d3()
            .then(draw_star_map)
            .then(function() {
                var deferred = $q.defer();
                interval = $interval(function() {
                    d3.select("#star_map").remove();
                    promise = promise.then(draw_star_map);
                }, 5000);
                deferred.resolve();
                return deferred.promise
            })

          }]
        return {
          restrict: 'EA',
          scope: {
          },
          controller: controller
        }
    }])
    .directive('gotoPage', ['$location', 'star_settings', function($location, star_settings) {
        return function(scope, element, attrs) {
            var path;

            attrs.$observe('gotoPage', function(val) {
                path = val;

            });
            element.bind('click', function() {
                if (path === "home") {
                    star_settings.map_on = true;
                } else {
                    star_settings.map_on = false;
                }
                scope.$apply(function() {
                    $location.path(path);
                });
            });
        }
    }])
})()