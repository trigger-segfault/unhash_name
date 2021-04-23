# unhash\_name #

A tool for brute-forcing recovery of CRC-32 hashed ASCII names, when supplied the resulting accumulator (CRC output) and optional static prefixes and postfixes.


## Usage

```bat
usage: unhash.exe <ACCUM> [PRE=] [POST=] [MAX=16] [MIN=0] [CHARSET=a-z_0-9]

arguments:
  ACCUM    target CRC-32 result to match (accepts hex prefix '0x' and '$')
  PRE      constant ASCII name prefix
  POST     constant ASCII name postfix
  MAX      maximum character length to test
  MIN      minimum character length to test
  CHARSET  list of characters for pattern (accepts ranges and '\' escapes)
```

Charset is defined similarly to a Regex range: *(`[a-z_]`)*. For example: `A-z` will be substituted with a range of all ASCII characters from `A` - `z`, *including* the symbols that appear between the upper and lowercase letters.

Characters *not* next to a dash will include only themselves. Use `\\` to define a backslash, and `\-` to define a dash. To work with the full ASCII range, use "` -~`" or "`!-~`" to skip space.

### Examples

```bat
C:\>unhash $a3d0623b "_start" "@" 7 1 "a-z_"
charset = "abcdefghijklmnopqrstuvwxyz_"
accum = 0xa3d0623b, init = 0x1541b913, target = 0x63083f04
depth = 1
depth = 2
depth = 3
depth = 4
"_starttime@"
depth = 5
depth = 6
depth = 7
^C
```

## Notes

### Optimizations

It's much easier to find an unhashed name by throwing common or expected keywords into the prefix and/or postfix.

The prefix **and now postfix** are only run through CRC-32 once, with that result being fed to -and compared with- all future calls to CRC-32 for the generated names. Both prefix and postfix can be as long as needed without impacting performance.

The best usage of this tool is not to run once and let it sit. It's recommended to make numerous attempts with manual changes to the constant prefixes, postfixes and character set, until potential results are found.

Trim out unlikely characters from the character set to increase the speed of each depth (length) calculation. This is most optimal when only one letter casing is used. e.g. *`snake_case`* and *`SCREAMING_SNAKE_CASE`*.

### Collisions

By depth 7 with a character set of `a-z_`, the number of calculations made already surpasses the size of a 32-bit unsigned integer:

`10460353203`<br>*vs.*<br>`4294967295`

Once you hit a length of 8 generated letters, the number of collisions becomes incredibly high. By length 9, it becomes unreasonable to sift through all the garbage collisions (unless your charset is relatively small).

