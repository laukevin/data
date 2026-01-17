# CLAUDE.md - Ember Data Development Guide

This document provides essential information for AI assistants working on the Ember Data codebase.

## Project Overview

**Ember Data** is a client-side data management library and ORM-like framework for Ember.js applications. It provides:

- A central **Data Store** for managing application state
- **Model** definitions with attributes and relationships
- **Adapters** for communicating with backend persistence layers (REST, ActiveModel, Fixture)
- **Serializers** for transforming data between the client and server
- **Record Arrays** and filtering mechanisms
- **Transforms** for data type conversions (String, Number, Date, Boolean)

**Version**: 1.0.0-beta.8+canary
**Namespace**: `DS`
**License**: MIT

## Directory Structure

```
/
├── packages/                       # Main source code (organized as packages)
│   ├── ember-data/                 # Core library
│   │   ├── lib/                    # Source code
│   │   │   ├── adapters/           # RESTAdapter, FixtureAdapter
│   │   │   ├── serializers/        # JSONSerializer, RESTSerializer
│   │   │   ├── system/             # Core modules
│   │   │   │   ├── model/          # Model definitions & attributes
│   │   │   │   ├── record_arrays/  # FilteredRecordArray, ManyArray, etc.
│   │   │   │   ├── relationships/  # belongsTo, hasMany
│   │   │   │   ├── changes/        # Change tracking
│   │   │   │   └── store.js        # Central store
│   │   │   ├── transforms/         # Data type transforms
│   │   │   └── main.js             # Main entry point
│   │   └── tests/                  # Unit & integration tests
│   ├── activemodel-adapter/        # Rails/ActiveModel adapter
│   └── ember-inflector/            # String inflection library
├── tasks/                          # Grunt build tasks
│   └── options/                    # Task configuration files
├── tests/                          # Global test configuration
├── docs/                           # Documentation assets
├── dist/                           # Built output (generated)
├── Gruntfile.js                    # Main build configuration
├── package.json                    # NPM dependencies
└── bower.json                      # Bower dependencies (Ember 1.4.0)
```

## Development Setup

### Prerequisites

- Node.js (version 0.10)
- PhantomJS (for CLI test running)

### Installation

```bash
npm install -g grunt-cli bower
npm install   # Also runs bower install via postinstall
```

## Build Commands

| Command | Description |
|---------|-------------|
| `grunt buildPackages` | Clean, transpile ES6 modules to AMD, concatenate, lint |
| `grunt test` | Build and run tests with PhantomJS (local Ember) |
| `grunt test:all` | Run tests against all Ember channels (local, release, beta, canary) |
| `grunt dev` | Start dev server with file watching at http://localhost:9997 |
| `grunt dist` | Build production-ready minified distribution |
| `grunt docs` | Generate API documentation with YUIDoc |

### Build Pipeline

1. **Transpilation**: ES6 modules → AMD format
2. **Concatenation**: Merge AMD modules with loader
3. **Defeatureify**: Feature flag stripping for production
4. **Minification**: Uglify.js for distribution
5. **Linting**: JSHint validation

## Testing

### Test Framework

- **QUnit** for unit and integration tests
- Tests located in `packages/ember-data/tests/`

### Running Tests

```bash
# Run all tests via CLI
grunt test

# Run tests in browser
grunt dev
# Then visit http://localhost:9997/tests

# Test against specific Ember version
grunt test:local    # Local Ember
grunt test:release  # Release channel
grunt test:beta     # Beta channel
grunt test:canary   # Canary channel
```

### Test Structure

```
packages/ember-data/tests/
├── unit/                   # Unit tests
│   ├── model/              # Model-specific tests
│   ├── store/              # Store tests
│   └── adapters/           # Adapter tests
└── integration/            # Integration tests
    ├── adapter/            # Adapter integration
    ├── relationships/      # Relationship tests
    ├── serializers/        # Serializer tests
    └── records/            # Record lifecycle tests
```

### Test Helpers

Key test utilities defined globally:

- `createStore()` - Creates a test store instance
- `setupStore()` - Sets up store with configuration
- `async()` - Wrapper for async test assertions
- `expectAssertion()` - Test for expected assertions
- `expectDeprecation()` - Test for expected deprecations

### Writing Tests

```javascript
var store;

module("unit/model - DS.Model", {
  setup: function() {
    store = createStore();
  },
  teardown: function() {
    store = null;
  }
});

test("description of behavior", function() {
  var record = store.createRecord(Person);
  // assertions using equal(), deepEqual(), ok(), strictEqual()
  equal(get(record, 'name'), 'expected', "assertion message");
});
```

## Code Style & Conventions

### Formatting

- **Two spaces** for indentation (no tabs)
- **No trailing whitespace**
- Blank lines should not have any space
- Use `a = b` not `a=b` (spaces around operators)
- Follow ES3+ compatible JavaScript

### ES6 Modules

The codebase uses ES6 module syntax, transpiled to AMD:

```javascript
// Importing
import {Store, PromiseArray} from "./system/store";
import RESTAdapter from "./adapters/rest_adapter";

// Exporting
export default Store;
export {Store, PromiseArray, PromiseObject};
```

### Ember Conventions

- Use `Ember.get()` and `Ember.set()` for property access
- Use `Ember.computed()` for computed properties
- Extend classes using `.extend()`
- Reopen classes using `.reopen()` or `.reopenClass()`

### Variable Naming

From the store.js comments:
- `id` - External identifier from the backend (always coerced to strings)
- `clientId` - Transient numerical identifier generated at runtime
- `reference` - Record reference object with metadata
- `type` - Subclass of DS.Model

### JSHint Configuration

Key linting rules (from `.jshintrc`):

- `es3: true` - ES3 compatible code
- `browser: true` - Browser environment
- `eqeqeq: true` - Strict equality (`===`)
- `undef: true` - No undefined variables
- `esnext: true` - ES6 features allowed (for modules)

Predefined globals: `Ember`, `DS`, `Handlebars`, `jQuery`, plus QUnit helpers

## Architecture Overview

### Core Components

1. **DS.Store** (`system/store.js`)
   - Central hub for data management
   - Methods: `find()`, `push()`, `createRecord()`, `filter()`
   - Manages record lifecycle and caching

2. **DS.Model** (`system/model/`)
   - Base class for data models
   - Defines attributes with `DS.attr()`
   - State machine for record states (clean, dirty, saving, etc.)

3. **DS.Adapter** (`system/adapter.js`)
   - Abstract interface for persistence
   - Subclasses: RESTAdapter, FixtureAdapter, ActiveModelAdapter

4. **DS.Serializer** (`serializers/`)
   - Transforms data between client and server formats
   - Subclasses: JSONSerializer, RESTSerializer

5. **Relationships** (`system/relationships/`)
   - `DS.belongsTo()` - One-to-one
   - `DS.hasMany()` - One-to-many

6. **Record Arrays** (`system/record_arrays/`)
   - `RecordArray` - Base array type
   - `FilteredRecordArray` - Client-side filtered records
   - `AdapterPopulatedRecordArray` - Records from adapter queries
   - `ManyArray` - hasMany relationship data

### Public API

All public classes are exported on the `DS` namespace:

```javascript
DS.Store, DS.Model, DS.attr
DS.RESTAdapter, DS.FixtureAdapter
DS.RESTSerializer, DS.JSONSerializer
DS.belongsTo, DS.hasMany
DS.Transform, DS.DateTransform, DS.NumberTransform, DS.StringTransform, DS.BooleanTransform
DS.ActiveModelAdapter, DS.ActiveModelSerializer
```

## Common Tasks

### Adding a New Feature

1. Create/modify files in `packages/ember-data/lib/`
2. Export from `main.js` if it's a public API
3. Add tests in `packages/ember-data/tests/`
4. Run `grunt test` to verify

### Fixing a Bug

1. Write a failing test that reproduces the issue
2. Implement the fix
3. Verify the test passes with `grunt test`

### Adding a New Adapter

1. Create adapter file in `packages/ember-data/lib/adapters/`
2. Extend `DS.Adapter` or `DS.RESTAdapter`
3. Implement required methods: `find()`, `findAll()`, `createRecord()`, etc.
4. Export from `adapters.js` and `main.js`

## Important Files

| File | Purpose |
|------|---------|
| `packages/ember-data/lib/main.js` | Main entry point, exports public API |
| `packages/ember-data/lib/system/store.js` | Central data store |
| `packages/ember-data/lib/system/model.js` | Base model class |
| `packages/ember-data/lib/adapters/rest_adapter.js` | Default REST adapter |
| `packages/ember-data/lib/serializers/rest_serializer.js` | REST serialization |
| `Gruntfile.js` | Build configuration |
| `tests/ember-data-setup.js` | Test helpers and synchronous test utilities |

## Additional Resources

- **README.md** - Project overview and quick start
- **CONTRIBUTING.md** - Contribution guidelines and PR process
- **TRANSITION.md** - Migration guide from 0.13 to 1.0
- **CHANGELOG.md** - Release notes and version history
- **Ember.js Guides**: http://emberjs.com/guides/models/
