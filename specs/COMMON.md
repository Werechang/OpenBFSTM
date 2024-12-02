# Common structures
Structures used in all sorts of files
## Section Info
| Offset | Type | Name      | Description           |
|--------|------|-----------|-----------------------|
| 0x0    | u16  | type/flag |                       |
| 0x2    | u16  | -         | padding               |
| 0x4    | s32  | offset    | Offset to the section |
| 0x8    | u32  | size      | Size of the section   |

## Reference Entry
| Offset | Type | Name      | Description         |
|--------|------|-----------|---------------------|
| 0x0    | u16  | type/flag |                     |
| 0x2    | u16  | -         | padding             |
| 0x4    | s32  | offset    | Offset to the Entry |