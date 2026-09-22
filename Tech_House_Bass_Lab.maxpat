{
  "patcher": {
    "fileversion": 1,
    "appversion": {
      "major": 9,
      "minor": 0,
      "revision": 0,
      "architecture": "x64"
    },
    "classnamespace": "box",
    "rect": [
      100,
      100,
      710,
      515
    ],
    "openinpresentation": 0,
    "default_fontsize": 12,
    "default_fontface": 0,
    "default_fontname": "Arial",
    "gridonopen": 1,
    "gridsize": [
      15,
      15
    ],
    "boxes": [
      {
        "box": {
          "id": "header",
          "maxclass": "comment",
          "patching_rect": [
            20,
            12,
            650,
            25
          ],
          "text": "TECH HOUSE BASS LAB V1 | MIDI effect | 16-step patterns | F1 default"
        }
      },
      {
        "box": {
          "id": "help",
          "maxclass": "comment",
          "patching_rect": [
            20,
            43,
            670,
            20
          ],
          "text": "Place before Serum 2. Start transport, then enable RUN. Change controls and click GENERATE."
        }
      },
      {
        "box": {
          "id": "transport",
          "maxclass": "newobj",
          "patching_rect": [
            20,
            80,
            90,
            22
          ],
          "text": "transport"
        }
      },
      {
        "box": {
          "id": "run",
          "maxclass": "toggle",
          "patching_rect": [
            20,
            116,
            24,
            24
          ]
        }
      },
      {
        "box": {
          "id": "metro",
          "maxclass": "newobj",
          "patching_rect": [
            20,
            153,
            135,
            22
          ],
          "text": "metro 16n @active 1"
        }
      },
      {
        "box": {
          "id": "reset",
          "maxclass": "message",
          "patching_rect": [
            170,
            115,
            60,
            22
          ],
          "text": "reset"
        }
      },
      {
        "box": {
          "id": "generate",
          "maxclass": "message",
          "patching_rect": [
            240,
            115,
            80,
            22
          ],
          "text": "generate"
        }
      },
      {
        "box": {
          "id": "groovelabel",
          "maxclass": "comment",
          "patching_rect": [
            20,
            207,
            520,
            22
          ],
          "text": "GROOVE: 0 Rolling | 1 Offbeat | 2 Syncopated | 3 Minimal"
        }
      },
      {
        "box": {
          "id": "groove",
          "maxclass": "number",
          "patching_rect": [
            20,
            236,
            60,
            22
          ],
          "minimum": 0,
          "maximum": 3
        }
      },
      {
        "box": {
          "id": "grooveprep",
          "maxclass": "newobj",
          "patching_rect": [
            95,
            236,
            120,
            22
          ],
          "text": "prepend setgroove"
        }
      },
      {
        "box": {
          "id": "densitylabel",
          "maxclass": "comment",
          "patching_rect": [
            240,
            207,
            140,
            22
          ],
          "text": "DENSITY 0-100"
        }
      },
      {
        "box": {
          "id": "density",
          "maxclass": "number",
          "patching_rect": [
            240,
            236,
            60,
            22
          ],
          "minimum": 0,
          "maximum": 100
        }
      },
      {
        "box": {
          "id": "densityprep",
          "maxclass": "newobj",
          "patching_rect": [
            315,
            236,
            125,
            22
          ],
          "text": "prepend setdensity"
        }
      },
      {
        "box": {
          "id": "rootlabel",
          "maxclass": "comment",
          "patching_rect": [
            20,
            285,
            360,
            22
          ],
          "text": "ROOT MIDI: 29 = F1 (octave naming may vary)"
        }
      },
      {
        "box": {
          "id": "root",
          "maxclass": "number",
          "patching_rect": [
            20,
            315,
            60,
            22
          ],
          "minimum": 24,
          "maximum": 48
        }
      },
      {
        "box": {
          "id": "rootprep",
          "maxclass": "newobj",
          "patching_rect": [
            95,
            315,
            110,
            22
          ],
          "text": "prepend setroot"
        }
      },
      {
        "box": {
          "id": "js",
          "maxclass": "newobj",
          "patching_rect": [
            20,
            370,
            200,
            22
          ],
          "text": "js tech_house_bass.js"
        }
      },
      {
        "box": {
          "id": "makenote",
          "maxclass": "newobj",
          "patching_rect": [
            20,
            410,
            140,
            22
          ],
          "text": "makenote 100 105"
        }
      },
      {
        "box": {
          "id": "noteout",
          "maxclass": "newobj",
          "patching_rect": [
            20,
            450,
            100,
            22
          ],
          "text": "noteout"
        }
      },
      {
        "box": {
          "id": "status",
          "maxclass": "message",
          "patching_rect": [
            260,
            370,
            410,
            40
          ],
          "text": "status"
        }
      },
      {
        "box": {
          "id": "load",
          "maxclass": "newobj",
          "patching_rect": [
            480,
            215,
            75,
            22
          ],
          "text": "loadbang"
        }
      },
      {
        "box": {
          "id": "init",
          "maxclass": "message",
          "patching_rect": [
            480,
            246,
            100,
            22
          ],
          "text": "29, 65, 0"
        }
      },
      {
        "box": {
          "id": "loadroot",
          "maxclass": "message",
          "patching_rect": [
            480,
            277,
            45,
            22
          ],
          "text": "29"
        }
      },
      {
        "box": {
          "id": "loaddensity",
          "maxclass": "message",
          "patching_rect": [
            530,
            277,
            45,
            22
          ],
          "text": "65"
        }
      },
      {
        "box": {
          "id": "loadgroove",
          "maxclass": "message",
          "patching_rect": [
            580,
            277,
            45,
            22
          ],
          "text": "0"
        }
      }
    ],
    "lines": [
      {
        "patchline": {
          "source": [
            "run",
            0
          ],
          "destination": [
            "metro",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "metro",
            0
          ],
          "destination": [
            "js",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "reset",
            0
          ],
          "destination": [
            "js",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "generate",
            0
          ],
          "destination": [
            "js",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "groove",
            0
          ],
          "destination": [
            "grooveprep",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "grooveprep",
            0
          ],
          "destination": [
            "js",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "density",
            0
          ],
          "destination": [
            "densityprep",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "densityprep",
            0
          ],
          "destination": [
            "js",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "root",
            0
          ],
          "destination": [
            "rootprep",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "rootprep",
            0
          ],
          "destination": [
            "js",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "js",
            0
          ],
          "destination": [
            "makenote",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "makenote",
            0
          ],
          "destination": [
            "noteout",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "makenote",
            1
          ],
          "destination": [
            "noteout",
            1
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "js",
            1
          ],
          "destination": [
            "status",
            1
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "load",
            0
          ],
          "destination": [
            "loadroot",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "load",
            0
          ],
          "destination": [
            "loaddensity",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "load",
            0
          ],
          "destination": [
            "loadgroove",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "loadroot",
            0
          ],
          "destination": [
            "root",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "loaddensity",
            0
          ],
          "destination": [
            "density",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "loadgroove",
            0
          ],
          "destination": [
            "groove",
            0
          ]
        }
      }
    ],
    "dependency_cache": [
      {
        "name": "tech_house_bass.js",
        "type": "TEXT"
      }
    ]
  }
}