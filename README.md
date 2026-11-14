# cgrab

Miniature program that prints special comments `# !`, `// !` from source code files, including line numbers. Supports most languages.

I personally find this most useful for learning new syntax and keeping track of pseudocode in larger codebases.

I suggest using `!!`, `!!!`... for sub-bullets.

## Build

```
gcc -o cgrab main.c
```
## Usage

```
./cgrab # all files in the current directory + subdirs
./cgrab test # all files in this directory + subdirs
./cgrab main.c test/dataclasses.py # individual files
```

Example output:

```
====== dataclasses.py ======
[5] # ! Enums are great for assigning names to fixed, immutable values.
[6] # !! Make sure to type the names in all capitals.
[12] # ! @property: access class methods like regular attributes
...
```

> [!NOTE]
> - Indented comments are normalized for consistency.
> - Hidden files and folders (names starting with `.`) are skipped.

## License

MIT — see [LICENSE](LICENSE).
