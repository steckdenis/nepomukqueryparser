# Parser for human-entered search queries

This repository contains some code used to experiment how a nice and powerful Nepomuk query parser can be built.

Nepomuk is the implementation of the Semantic Desktop used by KDE. It indexes files, e-mails, contacts, to-do's, notes, etc, and allows the user to query them. These queries can be automatic (Dolphin can show the documents created or edited yesterday or last week) or directly entered by the user.

## Parsing method

This project aims to produce a parser that is simple yet powerful, and more importantly easily translatable into many languages. The parser does not use a formal grammar but instead applies a set of passes on the input query, each pass replacing a set of Nepomuk query terms with new ones. For instance, the query

> mails sent by Richard last Monday

is first split in words (each locale can have a different set of space characters), then each word is converted into a `Nepomuk2::Query::LiteralTerm`. Now, parsing passes are applied on this list of terms:

* `mails` is recognized as being a keyword indicating that the user is looking for mails, and not files, videos, contacts, etc.
* `sent by X` is matched and replaced by a ComparisonTerm that compares the nmo:sender properties of the emails with "Richard".
* `last X` is matched and replaced by a filter on the date at which the emails have been received.

The final query becomes (if Today is June 15th, 2013)

> (ResourceType=nmo:Email) (nmo:messageFrom=Richard) (nmo:receivedDate=2013-06-10)

The last step, "last X", is not yet present in the repository and I still have to find out how to see that nmo:receivedDate must be used, and not `nfo:fileCreated` for instance.

## Translation

This parser uses simple `i18nc()` calls and is not relying on a per-locale XML file like [KHumanDateTime](https://github.com/steckdenis/khumandatetime) does. The rules are hard-coded in C++, and each language can provide a set of patterns for these rules. This allows complex rules to be used, they are not limited by the expressiveness of the XML file.

For instance, the `sent by` rule is coded like this in the source code:

```cpp
d->runPass(d->pass_sentby,
    i18nc("Sender of an email", "sent by <string0>|from <string0>"), 1);
```

`<string0>` is the word to be captured by the rule, and passed as parameter to the class implementing it. The different patterns that match the rule are separated by pipes. This allows other languages to have more or less rules than in English.