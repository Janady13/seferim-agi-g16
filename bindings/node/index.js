'use strict'
const path = require('path')
// Expect native addon built with node-gyp linking against ../../build/libg16_cabi.a
let native
try {
  native = require('node-gyp-build')(path.join(__dirname))
} catch (e) {
  throw new Error('Build the addon with node-gyp; ensure libg16_cabi.a exists in ../../build')
}
module.exports = native

