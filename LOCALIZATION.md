
# How to localize Knights to a new language

  - Consult https://partner.steamgames.com/doc/store/localization/languages and look at the "API language code" column to find the correct name for your new language.

  - Create a new file `knights_data/client/localization_NAME.txt`, where `NAME` is the language name (in lowercase). Translate all the strings, using [localization_english.txt](https://github.com/sdthompson1/knights/blob/main/knights_data/client/localization_english.txt) as the starting point.

    - The format of that file should be fairly self-explanatory, but if not, further details are available under [Localization](https://www.knightsgame.org.uk/lua/localization) in the Knights Lua docs.

    - Unicode characters are acceptable. The file should use UTF-8 encoding.

    - In `localization_english.txt`, two plural forms are used: `[one]` (for singular) and `[other]` (for plural). Other languages may require different plural forms. For this, the C++ code would have to be updated: see `Localization::pluralize` in [localization.cpp](https://github.com/sdthompson1/knights/blob/main/src/misc/localization.cpp).

    - Similarly, the list generation logic (which generates strings like `"A, B and C are the winners!"`) will need to be updated for other languages. See `Localization::buildList` in [localization.cpp](https://github.com/sdthompson1/knights/blob/main/src/misc/localization.cpp). Currently this hard-codes the list punctuation (comma placement rules) as well as the English word "and" -- maybe this should be generalized to handle other languages better.

  - Translate the credits text (from `knights_data/client/credits_english.txt`) into the new language, creating a new file `knights_data/client/credits_NAME.txt` (where NAME is the language name).

  - The Steam store settings must be updated, to tell Steam that the new language is supported. (This is only a few mouse clicks, but it can only be done by the owner of the Steam page.)


# Things that aren't done yet

Aside from the translations themselves, the following localization tasks are still TODO:

  - The key names (strings like "Page Down" or "Space") are not localizable yet.
     - We are just using the SDL function `SDL_GetKeyName` which returns localized names in some cases (e.g. for the single-letter keys) but English names in other cases (e.g. "Page Up").
     - This affects the Options screen (where you can view and change the control keys), the in-game message "Press Tab or ` to chat", and some Tutorial messages.
     - If there was demand for it, we could potentially add a database of common keys and their localized names in each language. But this has not been done so far.

  - The ESC in-game menu uses some shortcut keys such as "Q" to quit or "R" to restart. It might make sense to allow these letters to be changed in different languages.

  - The "Winner" and "Loser" images contain text. This should ideally be translated, but the game doesn't contain any mechanism for loading different image files based on language, so we are stuck with the English versions for now. Also, of course, we would need an artist (or someone with MS Paint skills!) to make the new images.

  - The Steam store page isn't done yet, but when it is, the text will have to be translated to different languages, clearly.

  - Team chat uses the prefix "/t". The "t" obviously comes from English, but maybe it would make sense to use another letter for other languages. (Alternatively, we could change the UI for how team chat works, or we could just leave it as it is - it's pretty minor.)
