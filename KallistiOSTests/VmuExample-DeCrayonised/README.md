# Crayon Vmu

There are two versions of the crayon vmu system. The old version has no backwards compatibility or portablility, but is easiest to use. The new versions supports backwards compatibility and portability, but is a little weird to use. This all stems from the fact that C has no easy way to serialise a struct so the old method was flawed and the new method fully works.

NOTE: If you only intend your savefiles to exist on one architecture or don't care about supporting savefile transfers, then the old version is probably for you.
