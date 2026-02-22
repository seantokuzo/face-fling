# Spec Template

Copy this template when creating a new feature specification.

---

# SPEC-XXX: Feature Name

> **Status**: `draft` | `review` | `approved` | `implemented` | `deprecated`
> **Author**: [Your Name]
> **Created**: YYYY-MM-DD
> **Updated**: YYYY-MM-DD

## Summary

One-paragraph description of what this feature does and why it's needed.

## Motivation

- Why is this feature necessary?
- What problem does it solve?
- What user need does it address?

## Requirements

### Functional Requirements

| ID   | Requirement                | Priority |
| ---- | -------------------------- | -------- |
| FR-1 | Description of requirement | P0/P1/P2 |
| FR-2 | ...                        | ...      |

### Non-Functional Requirements

| ID    | Requirement        | Target |
| ----- | ------------------ | ------ |
| NFR-1 | Performance target | X ms   |
| NFR-2 | Memory usage       | X MB   |

## Design

### API / Interface

```cpp
// Public interface for this feature
class FeatureName {
public:
    // Method descriptions
    ReturnType method_name(ParamType param);
};
```

### Data Structures

```cpp
// Any new data structures needed
struct NewStruct {
    Type field;
};
```

### Algorithm

Describe the algorithm or logic in detail. Include:

- Step-by-step process
- Edge cases
- Error handling

### Dependencies

- List external dependencies
- List internal components needed

## UI/UX (if applicable)

### Wireframe

```
┌─────────────────────────────────┐
│  Rough ASCII wireframe          │
│  of the UI                      │
└─────────────────────────────────┘
```

### User Flow

1. User does X
2. System responds with Y
3. User can then Z

## Test Cases

| ID   | Description | Input | Expected Output |
| ---- | ----------- | ----- | --------------- |
| TC-1 | Happy path  | ...   | ...             |
| TC-2 | Edge case   | ...   | ...             |
| TC-3 | Error case  | ...   | Error message   |

## Open Questions

- [ ] Question that needs discussion
- [ ] Design decision pending

## Implementation Notes

Notes for the implementer:

- Things to watch out for
- Performance considerations
- Related code to reference

## References

- Link to related specs
- External documentation
- Research/design docs
