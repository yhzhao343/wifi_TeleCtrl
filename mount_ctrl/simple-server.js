var portNum = 8080;
HOST = "localhost";
var SerialPort = require('serialport');
var exec = require('child_process').exec;
var static = require('node-static');
var parsers = SerialPort.parsers;
var file = new static.Server('./public');
var Q = require('q');
var server = require('http').createServer(serverhandle);
var io = require('socket.io')(server);
var gphoto2 = "gphoto2 ";
function serverhandle(req, resp) {
    req.addListener('end', function() {
        file.serve(req,resp);
    }).resume()
}
server.listen(portNum);
// SerialPort.list(function(err, ports) {
//     console.log("connect to : "  + ports[0].comName);
//     port = new SerialPort(ports[0].comName, {
//         baudRate:9600,
//         parser:parsers.readline('\n'),
//         lock:false
//     });
//     port.on('data', function(data) {
//         console.log(data);
//     })
//     port.on('error', function(err) {
//         console.log('Error: ' + err.message);
//     })
// });

io.on('connect', function(socket) {
    console.log('a user connected')
    var info = {port:{}};
    socket.on('focuser_mode', function(data) {
        var msg = {focuser:{mode:data}}
        console.log(msg);
    })
    // SerialPort.list(function(err, ports) {
    //     console.log("connect to : "  + ports[0].comName);
    //     var port = new SerialPort(ports[0].comName, {
    //         baudRate:9600,
    //         parser:parsers.readline('\n'),
    //         lock:false
    //     });
    //     info.port = port;
    //     port.on('data', function(data) {
    //         console.log(data);
    //     })
    //     port.on('error', function(err) {
    //         console.log('Error: ' + err.message);
    //     })
    //     port.on('open', function() {
    //         console.log('!!!')
    //         port.write(JSON.stringify({focuser:{mode:1}}) + '\n', function() {
    //             console.log('msg sent')
    //         })
    //     })


    // });
});

var port = new SerialPort('/dev/ttyACM0', {
    baudRate:9600,
    parser:parsers.readline('\n'),
    lock:false
});

port.on('error', function(err) {
    console.log('Error:' + err.message)
})
var queue = Q();
port.on('open', function() {
    console.log('serialport connected')
    queue = queue.then(function() {
        var deferred = Q.defer();
        port.write('HelloWorld!\n', function(err) {
            deferred.resolve();
            if (err) {
                console.log(err)
            }
            console.log('message written');
        })
        return deferred.promise;
    })
})

function send_json_msg(msg) {
    queue = queue.then(function() {
        var deferred = Q.defer();
        port.write(JSON.stringify(msg) + '\n', function() {
            deferred.resolve()
        });
        return deferred.promise;
    })
}

io.on('connect', function(socket) {
    console.log('a user connected')
    var info = {port:{}};
    socket.on('focuser_mode', function(data) {
        var msg = {focuser:{mode:data}}
        send_json_msg(msg);
    })
    socket.on('focuser', function(data) {
        console.log(data)
        send_json_msg(data);
    })
    socket.on('goto', function(data) {
        console.log(data)
        send_json_msg(data);
    })
    socket.on('set_tracking', function(data) {
        console.log(data)
        send_json_msg(data);
    })
    socket.on('set_slew', function(data) {
        console.log(data);
        send_json_msg(data);
    })
    socket.on('get_in_goto', function(data) {
        send_json_msg({telescope:{in_goto:{}, tracking:{}}});
    })
    socket.on('cancel_goto', function(data) {
        send_json_msg({telescope:{in_goto:false}});
    })
    socket.on('get_scope_pos', function(data) {
        send_json_msg({telescope:{EQ_Coord:{}}});
    })
    socket.on('command', function(data) {
        console.log(data);
        auto_detect(data.command);
    })
    function auto_detect(command) {
        console.log(command);
        console.log(gphoto2 + command);
        exec(gphoto2 + command, {cwd: '/home/pi/Projects/wifi_TeleCtrl/mount_ctrl/public/image/'}, emitStdout)
    }

    function emitStdout(err, stdout, stderr) {
        if (err) {
            console.log(err);
        };
        socket.emit('debug', stdout)
    }
    port.on('data', function(data) {
        console.log("recieve: " + data)
        try{
            data = JSON.parse(data)
            if (data) {
                if (data.telescope) {
                    if ((data.telescope.in_goto === true || data.telescope.in_goto === false) && (data.telescope.tracking || data.telescope.tracking === 0)) {
                        console.log(data.telescope);
                        socket.emit('in_goto_info', data.telescope)
                    }
                    if (data.telescope.EQ_Coord) {
                        console.log(data.telescope.EQ_Coord);
                        socket.emit('EQ_Coord', data.telescope.EQ_Coord);
                    };
                }
            }
        } catch(e) {
            console.log(data)
        }
    })

});







