(function() {
  'use strict'
  angular.module('teleCtrl.equipment')
  .service('cam', ['socket', function (socket) {
    var myCommandList = {
      autoDetect:'--auto-detect',
      listConfig:'--list-config'
    }

    var myShutterCmdList = {
      pressShutter:'--set-config eosremoterelease=5',
      releaseShutter:'--set-config eosremoterelease=4 --wait-event-and-download=2s'
    }

    var thisPointer = this;
    var functionFactory = function functionFactory(key, list) {
      var mySocket = socket
      console.log(list);
      thisPointer[key] = function() {
        mySocket.emit('command', {command:list[key]},
          function(result) {
            console.log(result);
          })
      }
    }
    this.commandList = myCommandList;
    this.shutter = myShutterCmdList;

    var cmdObjectArray = [myCommandList, myShutterCmdList];
    cmdObjectArray.forEach(function(cmdObject) {
      //var funcFactory = functionFactory;
      Object.keys(cmdObject).forEach(function(key) {
        functionFactory(key, cmdObject);
      })
    })
  }])
  // .service('telescope', ['socket', function (socket) {
  //   this.reply_message = "";
  //   var self = this;

  //   function send_msg(msg) {
  //     socket.emit('command', msg,
  //       function(result) {
  //         console.log(result);
  //         self.reply_message = result;
  //     })
  //   }
  //   // this.getProperties = function(){send_msg({"getProperties":{}});}
  //   // this.enableBLOB = function() {send_msg({"enableBLOB":{}})}
  //   // this.connect = function(){send_msg({"newSwitchVector":{"$":{"device":"EQMod Mount","name":"CONNECTION"},"oneSwitch":[{"_":" On","$":{"name":"CONNECT"}},{"_":" Off","$":{"name":"DISCONNECT"}}]}});}
  //   // this.goto = function(ra, dec) {send_msg({"newNumberVector":{"$":{"device":"EQMod Mount","name":"EQUATORIAL_EOD_COORD"},"oneNumber":[{"_":ra,"$":{"name":"RA"}},{"_":dec,"$":{"name":"DEC"}}]}})};
  // }])
})()