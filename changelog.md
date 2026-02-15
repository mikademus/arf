# Changelog

## [0.3.0] - 2026-02-13 - "Friday 13th Version"

### Major Changes
0.3.0 is a significant change compared to 0.2.0. While the Arf! format remains unchanged the support APIs have been completely redesigned and are substantially more powerful.

#### Architecture
 - arf.hpp was added as an entry point header
 - The document was split from arf_core.hpp to arf_document.hpp
 - Parsing (arf_parser.hpp) was split into separate concerns and files for parsing and materialisation
 - Materialisation validates all entities for structural and semantic correctness and propagates contamination
 - Parsing and materialisation emit errors and diagnostics that are stored next to the document
 - Reflection API was added (arf_reflect.hpp)
 - Editor API was added (arf_editor.hpp)

#### Document
- The document is now a full-fledged model where 0.2.0 was a map of category nodes
- The document retains authored order of all entities
- The document does not by default expose the internal data and provide immutable views. Internal nodes can be accessed through explicit accessors
- Extended contamination management API

#### Parser and materialiser
- The document is deserialised in separate steps:
  - Parsing traverse the source text and create a CST of source tokens
  - Materialisation create and populate the document from a CST event sequence and records the order of all entities

#### Reflection API - Complete Document Inspection 
**New module:** - The reflection API has been added. This proviodes full inspection into a document.

**Features**
Todo

#### Editor API - Complete CRUD Operations
**New module:** The editor API has been added. This provides programmatic editing of document entities.

**Features:**
- Full CRUD operations for all document entities (keys, comments, paragraphs, tables, rows, columns)
- Array element manipulation (append, set, erase elements within arrays)
- Type control methods (`set_key_type`, `set_column_type`)
- Column management (append, insert, erase columns with automatic row expansion)
- Row insertion (`insert_row_before/after`)
- Standardized contamination tracking across all mutation operations

#### Contamination Tracking
**Features:**
- Comprehensive contamination propagation for all edit operations
- Automatic contamination evaluation in array manipulation
- Column type changes properly invalidate affected rows

#### Serialisation
 - The serialiser retains the authored order of all entities
 - Created entities will be positioned in the authored structure

### Documentation

**Added:**
- Comprehensive editor test suite covering CRUD operations
- Integration tests demonstrating parse → query → edit → serialize workflows
- Query API documention (query_interface.md)
- Syntax guide for dotpath queries (query_dotpath_syntax.md)

### Deprecations

**None:** 0.3.0 is fully backward compatible with 0.2.0 for reading documents. The editor API is new, so no deprecations.

### Known Issues

- Moving entities (reordering within ordered_items) is not supported by design
- Column deletion does not shrink existing rows to match new column count *(if this is an issue)*
- Category creation/deletion not yet implemented

---

## [0.2.0] - 2025-XX-XX - Proof-of-Concept version
*TODO