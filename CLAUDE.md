# CLAUDE.md - AI Assistant Guide for Ember Data

## Project Overview

**Ember Data** is a data persistence library for Ember.js applications. It provides an ORM-like interface for loading data from a persistence layer (such as a JSON API), mapping data to models, updating models, and saving changes back to the server.

- **Version**: 1.0.0-beta (pre-release, API not stable)
- **Namespace**: `DS` (DataStore)
- **License**: MIT
- **Primary Use**: Client-side data management for Ember.js applications

## Directory Structure

```
/
├── packages/                     # Main source packages (modular structure)
│   ├── ember-data/              # Core Ember Data library
│   │   ├── lib/                 # Source code
│   │   │   ├── adapters/        # REST and Fixture adapters
│   │   │   ├── serializers/     # JSON and REST serializers
│   │   │   ├── system/          # Core system (store, model, relationships)
│   │   │   ├── transforms/      # Type transformations (date, number, etc.)
│   │   │   ├── initializers/    # Ember container initialization
│   │   │   └── main.js          # Primary entry point
│   │   └── tests/               # Tests organized by unit/integration
│   ├── activemodel-adapter/     # Rails ActiveModel adapter
│   └── ember-inflector/         # String inflection utilities
├── tasks/                        # Grunt build task definitions
│   └── options/                 # Individual task configurations
├── tests/                        # Test infrastructure and helpers
├── vendor/                       # Third-party libraries
├── docs/                         # Documentation assets
└── bin/                          # Executable scripts
```

## Key Architecture Concepts

### Core Components

| Component | Location | Purpose |
|-----------|----------|---------|
| **Store** | `packages/ember-data/lib/system/store.js` | Central data manager (singleton per app) |
| **Model** | `packages/ember-data/lib/system/model/` | Base class for all records |
| **Adapter** | `packages/ember-data/lib/system/adapter.js` | Translates between store and persistence layer |
| **Serializer** | `packages/ember-data/lib/serializers/` | Transforms between JS objects and API responses |
| **Transform** | `packages/ember-data/lib/transforms/` | Type coercion for model attributes |

### Adapters

- **RESTAdapter** (`adapters/rest_adapter.js`) - Default HTTP adapter for REST APIs
- **FixtureAdapter** (`adapters/fixture_adapter.js`) - In-memory adapter for testing/development
- **ActiveModelAdapter** (`activemodel-adapter/`) - Rails-specific conventions

### Record Arrays

Located in `packages/ember-data/lib/system/record_arrays/`:
- `RecordArray` - Base array for store queries
- `FilteredRecordArray` - Dynamically filtered subset
- `AdapterPopulatedRecordArray` - Adapter-filled (pagination-friendly)
- `ManyArray` - For hasMany relationships

### Relationships

Located in `packages/ember-data/lib/system/relationships/`:
- `belongsTo` - One-to-one relationships
- `hasMany` - One-to-many relationships

## Development Workflow

### Setup

```bash
# Install dependencies
npm install -g grunt-cli bower
npm install              # Also runs bower install via postinstall hook
```

### Common Commands

```bash
# Development server with auto-rebuild
grunt dev                # Starts server at http://localhost:9997/tests

# Run tests
grunt test               # Test against local ember
grunt test:all           # Test against all Ember versions (local, release, beta, canary)
grunt test:release       # Test against Ember release channel
grunt test:beta          # Test against Ember beta channel
grunt test:canary        # Test against Ember canary channel

# Build
grunt buildPackages      # Transpile ES6 to AMD, concatenate, lint
grunt dist               # Production build (minified, defeatureified)

# Documentation
grunt docs               # Generate YUIDoc documentation
```

### Build Pipeline

1. **setVersionStamp** - Stamps version from package.json
2. **clean** - Removes previous build artifacts
3. **transpile:amd** - Converts ES6 modules to AMD format
4. **concat:globals** - Concatenates files into global build
5. **browser:dist** - Creates browser distribution
6. **jshint** - Lints code for errors

## Testing Conventions

### Test Organization

```
packages/ember-data/tests/
├── integration/         # Full stack tests (adapters, serializers, relationships)
└── unit/                # Isolated unit tests (model, store, states)
```

### Test Helpers (defined in `tests/qunit_configuration.js`)

```javascript
// Create a test store with models
var store = createStore();

// Or with custom adapter
var store = setupStore({
  adapter: DS.RESTAdapter,
  person: Person  // Register models
});

// Async test helper with callback
store.find(Person, 1).then(async(function(person) {
  equal(person.get('name'), 'Tom');
}));

// Assert Ember assertion thrown
expectAssertion(function() { ... }, /regex/);

// Assert deprecation warning
expectDeprecation(function() { ... }, /regex/);
```

### Test File Pattern

```javascript
var get = Ember.get, set = Ember.set;

var Person, store;

module("unit/model - DS.Model", {
  setup: function() {
    store = createStore();
    Person = DS.Model.extend({
      name: DS.attr('string')
    });
  },
  teardown: function() {
    Person = null;
    store = null;
  }
});

test("descriptive test name", function() {
  var record = store.createRecord(Person);
  set(record, 'name', 'bar');
  equal(get(record, 'name'), 'bar', "assertion message");
});
```

### Running Tests in Browser

1. Start dev server: `grunt dev`
2. Visit: http://localhost:9997/tests
3. Optional params: `?package=all&jquery=2.0.0`

## Code Style Guidelines

### From `.jshintrc` and `CONTRIBUTING.md`

- **Indentation**: Two spaces, no tabs
- **Whitespace**: No trailing whitespace, blank lines should have no spaces
- **Operators**: `a = b` not `a=b` (spaces around operators)
- **Equality**: Use strict equality (`===`/`!==`) - `eqeqeq: true`
- **ES6**: ES6 module syntax supported (transpiled to AMD)
- **ES3 compat**: Maintain ES3 compatibility for IE8 support
- **Globals**: `Ember`, `DS`, `jQuery`, `Handlebars` predefined

### Module Pattern

```javascript
// ES6 import syntax (transpiled to AMD)
import DS from "./core";
import {Store, PromiseArray} from "./system/store";

// Export
export default DS;
export {Store, PromiseArray};
```

### Naming Conventions

- **Files**: `snake_case.js` (e.g., `rest_adapter.js`, `has_many.js`)
- **Classes**: PascalCase (e.g., `RESTAdapter`, `FilteredRecordArray`)
- **Methods/Properties**: camelCase (e.g., `findAll`, `isDirty`)
- **Private**: Prefix with underscore (e.g., `_reference`, `_setupContainer`)

### Ember Patterns Used

```javascript
// Computed properties
name: Ember.computed(function() { ... }).property('firstName', 'lastName')

// Observers
dataDidChange: Ember.observer(function() { ... }, 'data')

// Mixins
DS.EmbeddedRecordsMixin

// Promises
return Ember.RSVP.Promise.resolve(value);
```

## Common Tasks

### Adding a New Adapter

1. Create file in `packages/ember-data/lib/adapters/`
2. Extend `DS.Adapter` base class
3. Implement required methods: `find`, `findAll`, `findQuery`, `createRecord`, `updateRecord`, `deleteRecord`
4. Export from `packages/ember-data/lib/adapters.js`
5. Add to DS namespace in `packages/ember-data/lib/main.js`
6. Write tests in `packages/ember-data/tests/integration/adapter/`

### Adding a New Transform

1. Create file in `packages/ember-data/lib/transforms/`
2. Extend `DS.Transform` base class
3. Implement `serialize` and `deserialize` methods
4. Export from `packages/ember-data/lib/transforms.js`
5. Register in `packages/ember-data/lib/initializers/transforms.js`

### Adding a New Serializer

1. Create file in `packages/ember-data/lib/serializers/`
2. Extend `DS.JSONSerializer` or `DS.RESTSerializer`
3. Override normalization/serialization hooks as needed
4. Export and add to DS namespace in main.js

## Important Files Reference

| Purpose | File |
|---------|------|
| Main entry point | `packages/ember-data/lib/main.js` |
| Store implementation | `packages/ember-data/lib/system/store.js` |
| Model base class | `packages/ember-data/lib/system/model/model.js` |
| Model state machine | `packages/ember-data/lib/system/model/states.js` |
| REST adapter | `packages/ember-data/lib/adapters/rest_adapter.js` |
| REST serializer | `packages/ember-data/lib/serializers/rest_serializer.js` |
| Relationship definitions | `packages/ember-data/lib/system/relationships.js` |
| Container setup | `packages/ember-data/lib/setup-container.js` |
| Test helpers | `tests/qunit_configuration.js` |
| Build configuration | `Gruntfile.js` |
| JSHint rules | `.jshintrc` |

## Migration Notes

Breaking changes from previous versions are documented in `TRANSITION.md`. Key migrations include:

- DS.Model API changes
- Adapter method signatures
- Serializer hook names
- Relationship handling

Always consult `TRANSITION.md` when updating from older versions.

## CI/CD

- **Travis CI**: Runs tests via PhantomJS
- **Multi-version testing**: Tests against Ember release, beta, and canary channels
- **S3 deployment**: Builds deployed to S3 on successful CI

## Debugging

- **Debug Adapter**: `packages/ember-data/lib/system/debug.js` integrates with Ember Inspector
- **Development assertions**: Use `Ember.assert()` - stripped in production via defeatureify
- **Console logging**: `Ember.Logger.log()` for debug output
