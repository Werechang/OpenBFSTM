# BFSAR

Sound Archive File. WARNING: This format is over-engineered and a bit more complex.

## Header

| Offset | Type                       | Name        | Description                                                   |
|--------|----------------------------|-------------|---------------------------------------------------------------|
| 0x0    | char\[4\]                  | magic       | "FSAR"                                                        |
| 0x4    | u16                        | bom         | 0xfeff if the byte order should not be switched. 0xffef else. |
| 0x6    | u16                        | header_size | 0x40 in this version                                          |
| 0x8    | u32                        | version     | 0x00020400 (2.4.0) in our case                                |
| 0xc    | u32                        | file_size   |                                                               |
| 0x10   | u16                        | section_num | Number of section in the header. 0x3 here                     |
| 0x12   | u16                        | -           | padding                                                       |
| 0x14   | SectionInfo\[section_num\] | sections    | Types: 0x2000 -> STRG, 0x2001 -> INFO, 0x2002 -> FILE         |

As you can see, there is still some remaining space to fill the header size.

## STRG

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

### String Table

| Offset | Type                          | Name      | Description       |
|--------|-------------------------------|-----------|-------------------|
| 0x0    | u32                           | entry_num | Number of entries |
| 0x4    | StringTableEntry\[entry_num\] | entries   |                   |

#### String Table Entry

| Offset | Type | Name      | Description                                                 |
|--------|------|-----------|-------------------------------------------------------------|
| 0x0    | u16  | type/flag | 0x1f01                                                      |
| 0x2    | u16  | -         | padding                                                     |
| 0x4    | s32  | offset    | Offset to the string, relative from 0x0 in the String Table |
| 0x8    | u32  | size      | Size of the string including the null terminator            |

### Lookup Table

The lookup table (LUT) consists of tree entries referencing each other.

| Offset | Type                  | Name       | Description                   |
|--------|-----------------------|------------|-------------------------------|
| 0x0    | u32                   | root_index | Entry index of the root entry |
| 0x4    | u32                   | entry_num  |                               |
| 0x8    | LUTEntry\[entry_num\] | entries    |                               |

#### LUT Entry

| Offset | Type | Name               | Description                                                                    |
|--------|------|--------------------|--------------------------------------------------------------------------------|
| 0x0    | bool | endpoint           | If this entry is a leaf node. Only leaf nodes reference data (file and string) |
| 0x1    | u8   | -                  | padding                                                                        |
| 0x2    | u16  | compare_func       | See below                                                                      |
| 0x4    | u32  | left_child         | Entry index of the left child. 0xffffffff if is_leaf                           |
| 0x8    | u32  | right_child        | Entry index of the right child. 0xffffffff if is_leaf                          |
| 0xc    | u32  | string_table_index | Index of the corresponding string in the string table. 0xffffffff if !is_leaf  |
| 0x10   | u32  | item_id            | 0xTTIIIIII with T being the type and I being the index/identifier              |

The **compare function** is a special key with which the input string of the lookup function is compared.
The lookup function gets the file data (item_id) from a string which should match the string of that entry in the string
table.
The first 13 bits of the compare_func (`compare_func >> 3`) are used as an index into the name (with a bounds check).
The last 3 bits are used to see if the n-th bit of the character at that index is turned on (
`1 << (~compare_func & 0x7)`).
These bits are inverted for the comparison.

## INFO

| Offset | Type                | Name            | Description                                                        |
|--------|---------------------|-----------------|--------------------------------------------------------------------|
| 0x0    | char\[4\]           | magic           | "INFO"                                                             |
| 0x4    | u32                 | info_size       | Size of this section                                               |
| 0x8    | ReferenceEntry\[8\] | reference_table | the offsets in the entries are all relative to 0x8 in this section |

The order of the entries in the reference table is predetermined.
The types are listed below in that corresponding order:

| Type   | Type                      | Entry Type |
|--------|---------------------------|------------|
| 0x2100 | Sound Info                | 0x2200     |
| 0x2104 | Sound Group Info          | 0x2204     |
| 0x2101 | Bank Info                 | 0x2206     |
| 0x2103 | Wave Archive Info         |            |
| 0x2105 | Group Info                |            |
| 0x2102 | Player Info               |            |
| 0x2106 | File Info                 |            |
| 0x220B | Sound Archive Player Info |            |

Each of these info sections offset is relative to 0x8 in the info section.
Each one points to an instance of this structure:

| Offset | Type                        | Name      | Description                                                                                 |
|--------|-----------------------------|-----------|---------------------------------------------------------------------------------------------|
| 0x0    | u32                         | entry_num |                                                                                             |
| 0x2    | ReferenceEntry\[entry_num\] | entries   | The type of these entries is determined by the corresponding entry type in the table above. |

The offset of each reference entry is relative to 0x0 in the structure above.
Depending on the type (or entry type) of the specific info table entry, these entries are read as one of the following
structs.

### Sound Info Entry (0x2200)

| Offset | Type | Name           | Description                                                  |
|--------|------|----------------|--------------------------------------------------------------|
| 0x0    | u32  | file_id        | like item id but without file type                           |
| 0x4    | u32  | player_id      | always 0x04000000                                            |
| 0x8    | u8   | initial_volume |                                                              |
| 0x9    | u8   | remote_filter  | unused, always 0.                                            |
| 0xa    | u16  | -              | padding                                                      |
| 0xc    | u16  | sound_type     | 0x2201: prefetch/stream, 0x2202: Wave, 0x2203: sequence      |
| 0xe    | u16  | -              | padding                                                      |
| 0x10   | u32  | sound_info_off | Relative to 0x0 here, the type is defined by the sound type. |
| 0x14   | u32  | flags          | See below                                                    |
| 0x18   | ...  | flag_values    | See below                                                    |

**Flags (0x14) and flag values (0x18)**

Starting from 0x18 in the sound info entry, the values to the flags are stored.
Their offset is defined by the value of the previous flags. If for example 3 previous flags are on,
the offset is 3 ints (0xc) starting from 0x18.

| Flag Bit (0x14) | Name         | Flag Value (0x18)                                                            |
|-----------------|--------------|------------------------------------------------------------------------------|
| 0               | String Id    | Index of this entry in the string table                                      |
| 1               | Pan Info     | 0x????AABB With A defining the curve and B the mode                          |
| 2               | Player Prio  | Whole u32 is for the priority, the actor player id starts at the second byte |
| 3               | Single Play  | 0xAAAABBBB With B defining the type and A the effective duration             |
| 8               | Sound3D      | An offset to Sound3D Info, relative to 0x0 in this sound info entry.         |
| 18              | Front Bypass | If front bypass is enabled.                                                  |
| 32              | User Param   | ?                                                                            |

##### Stream Sound Info

| Offset | Type | Name                  | Description                                           |
|--------|------|-----------------------|-------------------------------------------------------|
| 0x0    | u16  | ?                     |                                                       |
| 0x2    | u16  | ?                     |                                                       |
| 0x4    | u16  | track_info_table_flag | 0x0101 if set                                         |
| 0x6    | u16  | ?                     |                                                       |
| 0x8    | s32  | track_info_table_off  | Relative to 0x0 in this structure                     |
| 0xc    | u32  | ?                     |                                                       |
| 0x10   | u32  | ?                     |                                                       |
| 0x14   | u32  | send_value            | Offset, relative to 0x0 here                          |
| 0x18   | u16  | stream_sound_ext_flag | 0x2210 if set                                         |
| 0x1a   | u16  | ?                     |                                                       |
| 0x1c   | s32  | stream_sound_ext_off  | 0xffffffff if not set, otherwise relative to 0x0 here |
| 0x20   | u32  | ?                     |                                                       |

**Send Value**
0xAABBBBCC

##### Wave Sound Info

| Offset | Type | Name                | Description |
|--------|------|---------------------|-------------|
| 0x0    | u32  | file_id             |             |
| 0x4    | u32  | ?                   |             |
| 0x8    | u8   | has_prio_info       |             |
| 0x9    | ...  | ?                   |             |
| 0xc    | u8   | channel_prio        |             |
| 0xd    | u8   | is_release_prio_fix |             |

##### Sequence Sound Info

| Offset | Type | Name | Description |
|--------|------|------|-------------|

#### Sound Group Info Entry (0x2204)

| Offset | Type | Name     | Description                                   |
|--------|------|----------|-----------------------------------------------|
| 0x0    | u32  | start_id |                                               |
| 0x4    | u32  | end_id   |                                               |
| 0x8    | u32  | ?        |                                               |
| 0xc    | u32  | ?_off    | Relative to 0x0 here                          |
| 0x10   | u16  | ?_flag   | 0x2205 if set, probably wave archive id table |
| 0x12   | u16  | ?        |                                               |
| 0x14   | u16  | ?_off    | 0xffffffff if not set, relative to ?          |

#### Bank Info Entry

| Offset | Type | Name                  | Description            |
|--------|------|-----------------------|------------------------|
| 0x0    | u32  | item_id               |                        |
| 0x4    | u32  | ?                     |                        |
| 0x8    | u32  | wave_arc_id_table_off | relative from 0x0 here |

#### Wave Archive Info Entry

| Offset | Type | Name     | Description                |
|--------|------|----------|----------------------------|
| 0x0    | u32  | item_id  |                            |
| 0x4    | u32  | file_num |                            |
| 0x8    | u32  | flags    | 0: string id 1: wave count |
| 0xc    | ...  | data     | specified by flags         |

#### Group Info Entry

| Offset | Type | Name            | Description |
|--------|------|-----------------|-------------|
| 0x0    | u32  | file_info_entry |             |

#### Player Info Entry

#### File Info Entry

| Offset | Type | Name          | Description                                                      |
|--------|------|---------------|------------------------------------------------------------------|
| 0x0    | u16  | location_type | 0x220d: external 0x220c: internal                                |
| 0x4    | s32  | info_offset   | relative from 0x0 here, points to internal or external file info |

##### Internal File Info

| Offset | Type | Name               | Description                                                      |
|--------|------|--------------------|------------------------------------------------------------------|
| 0x0    | u32  | ?                  |                                                                  |
| 0x4    | s32  | file_offset        | relative from 0x8 here (and relative from 0x8 in the file block) |
| 0x8    | u32  | ?                  |                                                                  |
| 0x10   | u32  | group_table_offset | relative from 0x0                                                |

##### External File Info

| Offset | Type  | Name               | Description             |
|--------|-------|--------------------|-------------------------|
| 0x0    | char* | external_file_name | Null terminated? string |

#### Sound Archive Player Info Entry

| Offset | Type | Name               | Description |
|--------|------|--------------------|-------------|
| 0x0    | u32  | sequence_num       |             |
| 0x4    | u32  | sequence_track_num |             |
| 0x8    | u32  | stream_num         |             |
| 0xc    | u16  | ?                  |             |
| 0xe    | u8   | ?                  |             |
| 0xf    | u8   | ?                  |             |
| 0x10   | u32  | ?                  |             |
| 0x14   | u32  | wave_num           |             |

## FILE
| Offset | Type      | Name      | Description          |
|--------|-----------|-----------|----------------------|
| 0x0    | char\[4\] | magic     | "FILE"               |
| 0x4    | u32       | file_size | Size of this section |
| 0x8    | ...       | -         | data                 |

All the data here is referenced in the info section.
