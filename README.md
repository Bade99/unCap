# unCap
A simple Windows app for removing SDH comments from subtitle files

### Win32 + C/C++
Everything is made right next to the base windows API, the main goal is to comprehend as much as possible about how Windows works and then do it yourself so you can customize anything and everything, handmade controls, non client, menus, renderer ...<br/>
The focus here is placed on Windows 10 but most things are useful for any version of the OS

### Goals
* Fully colorizable in realtime
* 100% hand painted and handled
* Framework for future projects

### Supported File Formats

* SubRip (.srt)
* Substation Alpha (.ssa)
* Advanced Substation Alpha (.ass)

### Supported File Encodings

* UTF-8
* UTF-8 BOM
* UTF-16LE
* UTF-16LE BOM
* UTF-16BE
* UTF-16BE BOM
* ANSI
* ASCII


### UI
![UI](https://i.imgur.com/DJ3VAFb.png)

### Completion (75% completed)
* Renderer: Images YES | Text NO
* Multilanguage: YES
* Non client Area: Handled YES | Drawn YES
* Single-Line Edit Control: Handled YES | Drawn YES
* Multi-Line Edit Control: Handled NO | Drawn YES
* Scrollbars: Handled YES | Drawn YES
* Buttons: Handled YES | Drawn YES
* Combo boxes: Handled NO | Drawn YES
* Static Text Control: Handled NO | Drawn YES
* Tab Control: Handled NO | Drawn YES
* End-User Editable Serialization: YES