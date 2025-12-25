# Tag Boundary Guidance

Use TagManager when:
- The tag is a shared runtime actor-state tag consumed across plugins
- The tag gates abilities/UI/stealth/interaction across module boundaries

Bypass TagManager is allowed when:
- Tag is purely local/internal to a single plugin and not observed by others

Always document boundary tags in the plugin’s docs and ensure the tag exists in DefaultGameplayTags.ini.
