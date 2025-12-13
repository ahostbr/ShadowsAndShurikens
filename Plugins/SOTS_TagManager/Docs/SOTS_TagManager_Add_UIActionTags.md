# SOTS UI Router Action Tags (Dec 2025)

## Where tags are authored
- Gameplay tags are authored via config: `Config/DefaultGameplayTags.ini` under `/Script/GameplayTags.GameplayTagsSettings`.
- The project also imports tag DataTables referenced in that config: `/ParkourSystem/DataTables/GameplayTagsDT` and `/Game/InventorySystemPro/Blueprints/Core/DT_InventoryTags`. These tables remain unchanged.
- `SOTS_TagManager` provides runtime access/helpers only; it does not author tags.

## Files edited
- `Config/DefaultGameplayTags.ini`

## Added tags
```
+GameplayTagList=(Tag="SAS.UI.Action.QuitGame",DevComment="SOTS_UI router action")
+GameplayTagList=(Tag="SAS.UI.Action.ReturnToMainMenu",DevComment="SOTS_UI router action")
+GameplayTagList=(Tag="SAS.UI.Action.OpenSettings",DevComment="SOTS_UI router action")
+GameplayTagList=(Tag="SAS.UI.Action.OpenProfiles",DevComment="SOTS_UI router action")
+GameplayTagList=(Tag="SAS.UI.Action.CloseTopModal",DevComment="SOTS_UI router action")
```

## Conflicts / redirects
- No existing tags with these names were present; no redirects added or required.

## Notes
- Tags are available in editor/runtime via the standard GameplayTags settings flow; no plugin binaries were touched for this change.
