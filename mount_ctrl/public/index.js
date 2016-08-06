(function() {
    angular.module('teleCtrl', ['teleCtrl.controller', 'teleCtrl.routes', 'teleCtrl.directives', 'teleCtrl.d3', 'teleCtrl.star_calc', 'teleCtrl.com']);
    angular.module('teleCtrl.controller', ['teleCtrl.equipment', 'teleCtrl.com', 'teleCtrl.d3', 'teleCtrl.star_database', 'teleCtrl.star_calc', 'teleCtrl.star_settings']);
    angular.module('teleCtrl.directives', ['teleCtrl.d3', 'teleCtrl.star_calc', 'teleCtrl.star_settings', 'teleCtrl.star_database']);
    angular.module('teleCtrl.star_calc',['teleCtrl.star_database', 'teleCtrl.star_settings']);
    angular.module('teleCtrl.equipment', ['teleCtrl.com']);
    angular.module('teleCtrl.d3', []);
    angular.module('teleCtrl.star_settings', []);
    angular.module('teleCtrl.star_database',[]);
    angular.module('teleCtrl.routes', []);
    angular.module('teleCtrl.com', []);
})();