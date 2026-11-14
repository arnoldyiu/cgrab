# cgrab

Miniature program that grabs special comments `# !`, `// !` from source code files (including line numbers) and saves the contents to a `.txt` or `.md` file.

I personally find this most useful for learning new syntax and keeping track of pseudocode in larger codebases.

I suggest using `!!`, `!!!`... for sub-bullets.

## Build

```
gcc -o cgrab main.c
```

## Usage

```
./cgrab                             # all files in the current directory + subdirs
./cgrab test                        # all files in this directory + subdirs
./cgrab ..                          # all files in the parent directory + subdirs
./cgrab main.c test/dataclasses.py  # individual files
```

Then, type your file name and press `Enter`.

Note that you *cannot* overwrite existing files. Choose a different name or delete the existing file.

```
Save to (.txt OR .md): study_notes.md
```

### Sample output

Example of `.txt` output:

```
====== test/dataclasses.py ======
[5] # ! Enums are great for assigning names to fixed, immutable values.
[6] # !! Make sure to type the names in all capitals.
[12] # ! @property: access class methods like regular attributes
[18] # ! @dataclass(frozen=True): prevent modification after instantiating the object

====== test/pointers.cpp ======
[3] // ! using namespace std to write cout instead of std::cout
[11] // ! &num "memory address of num"
...
```

Example of `.md` output:

````
### test/dataclasses.py
```
[5] # ! Enums are great for assigning names to fixed, immutable values.
[6] # !! Make sure to type the names in all capitals.
...
```
````



## Supported languages

| Comment style | File extensions |
|---|---|
| `# !` | `.py` `.rb` `.pl` `.r` `.sh` `.bash` `.zsh` `.yml` `.yaml` `.toml` |
| `// !` | `.c` `.h` `.cpp` `.hpp` `.cc` `.cxx` `.js` `.jsx` `.ts` `.tsx` `.java` `.go` `.rs` `.swift` `.kt` `.cs` `.php` `.scala` |

> [!NOTE]
> - Indented comments are normalized for consistency.
> - Files are listed in alphabetical order.
> - Files without the appropriate comment prefixes (see `test/helloworld.py`) are excluded.
> - Hidden files and folders (names starting with `.`) are skipped.
> - File extensions must be in lowercase.

## License

[MIT](LICENSE)