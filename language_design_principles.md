# Catalyst Language Design Principles

## Criteria in the design

- **Readability**\
  A piece of code should be easy to read and the reader should be able to understand and comprehend the code quickly and accurately.
- **Write-ability**\
  Writing code for any general purpose should be:
  - _clearly_: directions toward a solution to a problem should be straight forward 
  - _correctly_: that direction by definition is idiomatic and ultimately 'the right way'. There should not be any ambiguity.
  - _concisely_: writing code for all intentions should be brief but explicit. No fluff.
  - _quickly_: without violating any of the other criteria, code should be compact. Many/most traditionally longer keywords are shorthands (like `fn` instead of ~~`function`~~) or symbols (`:` instead of ~~`implements`~~). In doubt or conflict, the criterea for _clearly_ has priority.
- **Expressivity**\
  The language should allow us to construct as wide a variety of programs as possible.
- **Reliability**\
  Assures a program will not behave in unexpected ways. Writing code that has 'undefined behavior' should be impossible. (_should_ means that it should never happen, which is a claim we can never guarantee. Any 'undefined behavior' is a bug and should be reported as such.)
- **Orthogonality**
  - A relatively small set of primitive constructs can be combined in a relatively small number of ways
  - Every possible combination is legal
  - Lack of orthogonality leads to exceptions to rules
- **Uniformity**\
  Similar features should look similar and behave similar.
- **Maintainability**\
  Errors can be found and corrected and new features added easily.
- **Generality**\
  Avoid special cases in the availability or use of constructs and by combining closely related constructs into a single more general one
- **Extensibility**\
  Provide some general mechanism for the user to add new constructs to a language.
- **Standardability**\
  Allow programs to be transported from one computer to another without significant change in language structure.
- **Implementability**\
  Ensure a translator or interpreter can be written. The grammar should be context-free.
- **Safety**\
  Mechanisms should be available to allow errors to be detected.
- **Efficiency**\
  The language should not preclude the production of efficient code.
- **Modularity**\
  Parts of a program can be compiled separately from the rest.

## Statement syntax
The syntax for statements should _generally_ follow a predictable pattern:
 > _`designator`_ ⁔ _`subject`_ ⁔ _`operation`?_

where:\
_`designator`_: keyword that identifies the type of statement;\
_`subject`_: the subject this statement is designated to;\
_`operation`_: optional[^1] part that defines the operation to attach to the statement.

[^1]: Optional in the sense of language design. The _`operation`_ part can still be required in a specific stype of statement.

Please note the word '_generally_' in the initial sentence of this section. This means that, if that would make sense to the criteria above, syntax might deviate from this rule to accomodate the satisfaction of more (or more important) criteria.

### Declarations

|Type|Syntax|Example|Justification|
| --- | --- | --- | --- |
|Variable Declaration | `var` `identifier` (`->` `type`)? (`=` `expression`)? | `var index = 5` | 
