import unreal

asset_path = "/Game/DevTools/BP_PrintHello"
output_path = "E:/SAS/ShadowsAndShurikens/DevTools/python/bp_functions_out.txt"


def write_attrs(outfile, label, obj):
    attrs = sorted(set(dir(obj)))
    outfile.write(f"{label}:\n")
    outfile.write("\n".join(attrs))
    outfile.write("\n\n")


def safe_call(label, func, outfile):
    try:
        result = func()
        outfile.write(f"{label}: {len(result)} entries\n")
        for entry in result:
            outfile.write(f"  {entry}\n")
        outfile.write("\n")
    except Exception as exc:
        outfile.write(f"{label} raised {exc}\n\n")


blueprint = unreal.EditorAssetLibrary.load_asset(asset_path)
if not blueprint:
    unreal.log_error(f"Failed to load blueprint asset at {asset_path}")
    with open(output_path, "w", encoding="utf-8") as outfile:
        outfile.write("Blueprint asset missing")
else:
    generated_class = blueprint.generated_class
    with open(output_path, "w", encoding="utf-8") as outfile:
        write_attrs(outfile, "Blueprint attributes", blueprint)
        if generated_class:
            write_attrs(outfile, "Generated class attributes", generated_class)
        else:
            outfile.write("Blueprint has no generated class\n\n")

        if hasattr(blueprint, "get_all_graphs"):
            safe_call("Blueprint graphs", lambda: blueprint.get_all_graphs(), outfile)
        else:
            outfile.write("Blueprint.get_all_graphs not available\n\n")

        if hasattr(blueprint, "get_editor_property"):
            try:
                func_graphs = blueprint.get_editor_property("FunctionGraphs")
                outfile.write(f"Blueprint FunctionGraphs: {len(func_graphs)} entries\n")
                for graph in func_graphs:
                    name = getattr(graph, "get_name", lambda: getattr(graph, "_name", "<unnamed>"))()
                    outfile.write(f"  {name}\n")
                outfile.write("\n")
            except Exception as exc:
                outfile.write(f"Blueprint FunctionGraphs access failed: {exc}\n\n")
        else:
            outfile.write("Blueprint lacks get_editor_property\n\n")

        write_attrs(outfile, "BlueprintEditorLibrary attributes", unreal.BlueprintEditorLibrary)

        candidate_calls = [
            ("get_all_functions", lambda: unreal.BlueprintEditorLibrary.get_all_functions(blueprint)),
            ("get_all_graphs", lambda: unreal.BlueprintEditorLibrary.get_all_graphs(blueprint)),
            ("get_blueprint_functions", lambda: unreal.BlueprintEditorLibrary.get_blueprint_functions(blueprint)),
            ("get_function_callers", lambda: unreal.BlueprintEditorLibrary.get_function_callers(blueprint)),
        ]
        for label, func in candidate_calls:
            if hasattr(unreal.BlueprintEditorLibrary, label):
                safe_call(label, func, outfile)
            else:
                outfile.write(f"BlueprintEditorLibrary.{label} not available\n\n")

        if generated_class and hasattr(generated_class, "get_editor_property"):
            try:
                func_graphs = generated_class.get_editor_property("FunctionGraphs")
                outfile.write(f"Generated class FunctionGraphs: {len(func_graphs)} entries\n")
                for graph in func_graphs:
                    name = getattr(graph, "get_name", lambda: getattr(graph, "_name", "<unnamed>"))()
                    outfile.write(f"  {name}\n")
                outfile.write("\n")
            except Exception as exc:
                outfile.write(f"Accessing FunctionGraphs failed: {exc}\n\n")
        elif generated_class:
            outfile.write("Generated class lacks get_editor_property\n\n")
