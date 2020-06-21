# SF Versions

The new version of the savefile system supports multiple revisions of your savedata, aka backwards compatibility. This means you can add new variables, remove old ones and perform a few basic operations for handling deleted variables.

If you make a savefile in v1, then it will load in v2, v3, etc, but a savefile in v3 won't load in v1/v2 and a v2 savefile won't load in v1.

