cbor = require('bindings')('svar')('svar_cbor')

buf = cbor.dump_cbor({"hello":1})

console.log(cbor.parse_cbor(buf))
