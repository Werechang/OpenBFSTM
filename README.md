# OpenBFSTM
## Overview
This program will be able to decode and encode
different kinds of audio files used in Super Mario Odyssey.

| File  | Decode | Encode | File Versions | Compatibility Notes                                                                    |
|-------|--------|--------|---------------|----------------------------------------------------------------------------------------|
| BFSTM | ✅      | ⬜      | 6.1.0         | Encoding is not 100% correct since the calculated dspadpcm coefficients are different. |
| BFGRP | ✅      | ✅      | 1.0.0         |                                                                                        |
| BFSAR | ✅      | ⬜      | 2.4.0, 2.2.0  | There are still some unknown variables + missing things.                               |
| BFSTP | ⬜      | ⬜      |               |                                                                                        |
| BFWAR | ✅      | ✅      | 1.0.0         | Encoding: Small differences in alignment algorithm                                     |
| BFWAV | ⬜      | ⬜      |               |                                                                                        |
| BFWSD | ⬜      | ⬜      |               |                                                                                        |
| BFBNK | ⬜      | ⬜      |               |                                                                                        |
| BFSEQ | ⬜      | ⬜      |               |                                                                                        |

## Credits
- imgui
- implot
- https://mk8.tockdom.com
- https://github.com/Thealexbarney/VGAudio
- https://github.com/kinnay/Nintendo-File-Formats