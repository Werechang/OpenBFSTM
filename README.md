# OpenBFSTM
## Overview
This program will be able to decode and encode
different kinds of audio files used in Super Mario Odyssey.

| File  | Decode  | Encode | File Versions | Compatibility Notes |
|-------|---------|--------|---------------|---------------------|
| BFSTM | ✅       | ⬜      | 6.1.0         |                     |
| BFGRP | ⬜       | ⬜      | 1.0.0         |                     |
| BFSAR | ⬜       | ⬜      | 2.4.0         |                     |

## Specifications
These are the specs for the file types with versions above.
### BFSAR
#### Header
| Offset | Type                       | Name        | Description                                                   |
|--------|----------------------------|-------------|---------------------------------------------------------------|
| 0x0    | char\[4\]                  | magic       | "FSAR"                                                        |
| 0x4    | u16                        | bom         | 0xfeff if the byte order should not be switched. 0xffef else. |
| 0x6    | u16                        | header_size | 0x40 in this version                                          |
| 0x8    | u32                        | version     | 0x00020400 (2.4.0) in our case                                |
| 0xc    | u32                        | file_size   |                                                               |
| 0x10   | u16                        | section_num | Number of section in the header. 0x3 here                     |
| 0x12   | u16                        | -           | padding                                                       |
| 0x14   | SectionInfo\[section_num\] | sections    | Flags: 0x2000 -> STRG, 0x2001 -> INFO, 0x2002 -> FILE         |
As you can see, there is still some remaining space to fill the header size.

##### Section Info
| Offset | Type | Name      | Description                    |
|--------|------|-----------|--------------------------------|
| 0x0    | u16  | type/flag |                                |
| 0x2    | u16  | -         | padding                        |
| 0x4    | s32  | offset    | Absolute offset to the section |
| 0x8    | u32  | size      | Size of the section            |

#### STRG
This section points to 2 structures, a string table which contains an indexed list of strings and
a lookup table which is a binary tree.

| Offset | Type      | Name                | Description                         |
|--------|-----------|---------------------|-------------------------------------|
| 0x0    | char\[4\] | magic               | "STRG"                              |
| 0x4    | u32       | strg_size           | Size of this section                |
| 0x8    | u16       | string_table_flag   | 0x2400                              |
| 0xa    | u16       | -                   | padding                             |
| 0xc    | s32       | string_table_offset | Relative from 0x8 in this structure |
| 0x10   | u16       | lookup_table_flag   | 0x2401                              |
| 0x12   | u16       | -                   | padding                             |
| 0x14   | s32       | lookup_table_offset | Relative from 0x8 in this structure |
##### String Table
| Offset | Type                          | Name      | Description       |
|--------|-------------------------------|-----------|-------------------|
| 0x0    | u32                           | entry_num | Number of entries |
| 0x4    | StringTableEntry\[entry_num\] | entries   |                   |
###### String Table Entry
| Offset | Type | Name       | Description                                                 |
|--------|------|------------|-------------------------------------------------------------|
| 0x0    | u16  | magic/type | 0x1f01                                                      |
| 0x2    | u16  | -          | padding                                                     |
| 0x4    | s32  | offset     | Offset to the string, relative from 0x0 in the String Table |
| 0x8    | u32  | size       | Size of the string including the null terminator            |
##### Lookup Table
The lookup table (LUT) consists of tree entries referencing each other.

| Offset | Type                  | Name       | Description                   |
|--------|-----------------------|------------|-------------------------------|
| 0x0    | u32                   | root_index | Entry index of the root entry |
| 0x4    | u32                   | entry_num  |                               |
| 0x8    | LUTEntry\[entry_num\] | entry_num  |                               |
###### LUT Entry
| Offset | Type | Name               | Description                                           |
|--------|------|--------------------|-------------------------------------------------------|
| 0x0    | bool | is_leaf            | If this entry is a leaf node.                         |
| 0x1    | u8   | -                  | padding                                               |
| 0x2    | u16  | compare_func       |                                                       |
| 0x4    | u32  | left_child         | Entry index of the left child                         |
| 0x8    | u32  | right_child        | Entry index of the right child                        |
| 0xc    | u32  | string_table_index | Index of the corresponding string in the string table |
| 0x10   | u32  | item_id            |                                                       |
#### INFO
#### FILE

## Credits
- imgui
- implot
- https://mk8.tockdom.com
- https://github.com/Thealexbarney/VGAudio