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

## Translation

This parser uses simple `i18nc()` calls and is not relying on a per-locale XML file like [KHumanDateTime](https://github.com/steckdenis/khumandatetime) does. The rules are hard-coded in C++, and each language can provide a set of patterns for these rules. This allows complex rules to be used, they are not limited by the expressiveness of the XML file.

For instance, the `sent by` rule is coded like this in the source code:

```cpp
d->runPass(d->pass_sentby,
    i18nc("Sender of an email", "sent by %1;from %1"));
```

`%1` is the word to be captured by the rule, and passed as parameter to the class implementing it. The different patterns that match the rule are separated by semicolons. This allows other languages to have more or less rules than English.

## C++ passes

A C++ pass is a class that exposes a `run()` method. The class does not have to inherit from another one, as the pattern matcher (the component that runs rules against matched patterns) uses templates. The method must have the following signature:

```cpp
QList<Nepomuk2::Query::Term> run(const QList<Nepomuk2::Query::Term> &match) const;
```

`match` is the list of matched terms. For instance, `match.at(0)` contains the pattern matched by "%1", and `match.at(1)` contains the one matched by "%2".

The method returns a list of terms that will replace the **entire match**, literal values included. So, if "sent to %1" is matched and %1 is a contact name, a pass can return a `ComparisonTerm`, that will replace the three terms matched.

This means that if a pattern matches "sent to %1", no other pattern can match "sent to %1 but not to %2", as the first pattern already *consumed* the beginning of the second pattern, that is therefore unable to match anything. The order of patterns is important, you must begin with the longer ones (first try to match "2013-04-04" then "2013-04").
