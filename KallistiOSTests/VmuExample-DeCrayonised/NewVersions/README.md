# SF Versions

The new version of the savefile system supports multiple revisions of your savedata, aka backwards compatibility. This means you can add new variables, remove old ones and perform a few basic operations for handling deleted variables.

If you make a savefile in v1, then it will load in v2, v3, etc, but a savefile in v3 won't load in v1/v2 and a v2 savefile won't load in v1.

### Savefile structure

A file is generally the header + body. So here's the general idea of how it works. For PC we have header info first, then the version number currently set to two bytes (unsigned) (This can be changed in **savefile.h**), then the user's savedata. 

For Dreamcast its not important to know the order of stuff in the body (ie. what order do the eyecatcher, savefile icon, version number and user data go in) since the version number is always just before the user data (Technically its a part of the user data according to the Dreamcast, but you can't modify it in the `crayon_savefile_data_t` struct)
