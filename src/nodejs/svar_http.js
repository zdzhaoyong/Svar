s = require('bindings')('svar')('svar_http')

api = {"add":function(msg){return "hello"}}

server = new s.Server('0.0.0.0',1024,api)

function sleep (time) {
  return new Promise((resolve) => setTimeout(resolve, time));
}

sleep(100000)
