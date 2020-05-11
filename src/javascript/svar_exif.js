exif = require('bindings')('svar')('svar_exif')

meta = exif.read_meta('/data/zhaoyong/Dataset/RTMapper/campus/images/DJI_0361.JPG')

console.log(meta)
