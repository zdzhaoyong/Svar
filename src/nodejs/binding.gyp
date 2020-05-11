{
  "targets": [{
    "target_name": "svar",
    'includes': [
        'auto.gypi'
      ],

    "sources": ["./src/main.cpp"],
    'cflags_cc+': [
        '-std=c++11',
        '-frtti',
        '-Wno-deprecated-declarations',
        '-O3',
        '-fexceptions'
      ],
    'cflags_cc!': [
        '-fno-rtti',
        '-fno-exceptions'
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
  'msbuild_settings': {
        'ClCompile': {
          'AdditionalOptions': [
            # fatal error C1128: number of sections exceeded object file format limit:
            # compile with /bigobj
            '/bigobj'
          ]
        }
      },
      'conditions': [
        ['OS == "linux"', {
          'link_settings': {
            'libraries': [
            ]
          }
        }],
        ['OS == "win"', {
          'cflags_cc+': [
            '-stdlib=libc++'
          ],
          'link_settings': {
            'libraries': [
            ]
          }
        }]
      ]

  }]

}
