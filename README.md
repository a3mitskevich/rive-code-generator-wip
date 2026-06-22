<h1 align="center">
  <a href="https://github.com/rive-app/rive-code-generator-wip">
    Rive Code Generator
  </a>
</h1>

<p align="center">
  <strong>Simplify Rive integration at runtime:</strong><br>
  Generate type-safe wrappers for Rive files.
</p>

<p align="center">
  <a href="https://github.com/rive-app/rive-code-generator-wip/blob/main/LICENSE">
    <img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="Rive Code Generator is released under the MIT license." />
  </a>
  <a href="https://github.com/rive-app/rive-code-generator-wip/actions">
    <img src="https://github.com/rive-app/rive-code-generator-wip/actions/workflows/build_rive_code_generator.yaml/badge.svg" alt="Current GitHub Actions build status." />
  </a>
  <a href="https://github.com/rive-app/rive-code-generator-wip/releases">
    <img src="https://img.shields.io/github/v/release/rive-app/rive-code-generator-wip" alt="Latest GitHub release." />
  </a>
  <a href="https://github.com/rive-app/rive-code-generator-wip/blob/main/CONTRIBUTING.md">
    <img src="https://img.shields.io/badge/PRs-welcome-brightgreen.svg" alt="PRs welcome!" />
  </a>
</p>

<h3 align="center">
  <a href="#releases">Releases</a>
  <span> · </span>
  <a href="#usage">Usage</a>
  <span> · </span>
  <a href="#custom-templates">Custom Templates</a>
  <span> · </span>
  <a href="#contributing">Contribute</a>
  <span> · </span>
  <a href="#license">License</a>
</h3>

This tool parses Rive (`.riv`) files and extracts component names, artboards, state machine inputs, and other components in a human-readable format. Key features include:

- Creating type-safe wrappers for `.riv` files (e.g., static helper classes)
- Generating JSON representations of `.riv` files
- Diffing `.riv` files for version control purposes
- Generating complete code components

The tool uses [Mustache](https://mustache.github.io/) templating for flexible output generation.

:warning: Note that this tool is still experimental and untested. Feedback and contributions are appreciated.

## Releases

See the [release workflow](.github/workflows/release_workflow.yaml) for release details.

The latest release can be downloaded from the [Releases](https://github.com/rive-app/rive-code-generator-wip/releases) page.

## Usage

Run the code generator using:

```sh
./build/out/<debug|release>/rive_code_generator -i <input_rive_file> -o <output_directory> -l <language>
```

Example:

```sh
./build/out/release/rive_code_generator -i ./examples/rive_files/animation.riv -o ./examples/generated_code.dart -l dart
```

## Custom Templates

You can use custom Mustache templates for code generation:

```sh
./build/out/release/rive_code_generator -i ./rive_files/ -o ./output/rive.json -t templates/json_template.mustache
```

Sample templates are available in the [`templates`](./templates) directory.

### Template Syntax

The tool uses [Mustache](https://mustache.github.io/) templating. Please refer to the [Mustache documentation](https://mustache.github.io/) for syntax details.

For each Rive file `{{#riv_files}}`, the following variables are available:

- `{{riv_name}}`: The Rive file name
- `{{riv_pascal_case}}`: The Rive file name in PascalCase
- `{{riv_camel_case}}`: The Rive file name in camelCase
- `{{riv_snake_case}}`: The Rive file name in snake_case
- `{{riv_kebab_case}}`: The Rive file name in kebab-case
- `{{runtime_major_version}}`: Major version of the Rive runtime used to parse the file
- `{{runtime_minor_version}}`: Minor version of the Rive runtime used to parse the file

A top-level `{{generated_file_name}}` variable is also available (derived from the output path).

- `{{assets}}`: List of assets in the Rive file
- For each asset `{{#assets}}`:

  - `{{asset_name}}`: Name of the asset
  - `{{asset_camel_case}}`: Name of the asset in camelCase
  - `{{asset_pascal_case}}`: Name of the asset in PascalCase
  - `{{asset_snake_case}}`: Name of the asset in snake_case
  - `{{asset_kebab_case}}`: Name of the asset in kebab-case
  - `{{asset_type}}`: Type of the asset
  - `{{asset_id}}`: ID of the asset
  - `{{asset_width}}`: Width of the asset (for image assets)
  - `{{asset_height}}`: Height of the asset (for image assets)
  - `{{asset_is_embedded}}`: Whether the asset is embedded in the file
  - `{{asset_unique_filename}}`: Unique filename for the asset
  - `{{asset_cdn_uuid}}`: CDN UUID of the asset
  - `{{asset_cdn_base_url}}`: CDN base URL of the asset

- For each artboard `{{#artboards}}`:

  - `{{artboard_name}}`: The original artboard name
  - `{{artboard_pascal_case}}`: The artboard name in PascalCase
  - `{{artboard_camel_case}}`: The artboard name in camelCase
  - `{{artboard_snake_case}}`: The artboard name in snake_case
  - `{{artboard_kebab_case}}`: The artboard name in kebab-case
  - `{{artboard_width}}`: Width of the artboard
  - `{{artboard_height}}`: Height of the artboard
  - `{{artboard_origin_x}}`: X origin of the artboard
  - `{{artboard_origin_y}}`: Y origin of the artboard
  - `{{artboard_clip}}`: Whether the artboard clips its contents
  - `{{has_default_state_machine}}`: Whether the artboard has a default state machine
  - `{{default_state_machine_name}}` / `_camel_case` / `_pascal_case`: Default state machine name
  - `{{has_bound_view_model}}`: Whether the artboard is bound to a view model
  - `{{bound_view_model_name}}` / `_camel_case` / `_pascal_case`: Bound view model name
  - `{{animations}}`: List of animation names for the artboard
  - For each animation `{{#animations}}`:
    - `{{animation_name}}`: Name of the animation
    - `{{animation_camel_case}}`: Name of the animation in camelCase
    - `{{animation_pascal_case}}`: Name of the animation in PascalCase
    - `{{animation_snake_case}}`: Name of the animation in snake_case
    - `{{animation_kebab_case}}`: Name of the animation in kebab-case
    - `{{animation_fps}}`: Frames per second of the animation
    - `{{animation_duration_seconds}}`: Duration of the animation in seconds
    - `{{animation_loop}}`: Loop mode of the animation
    - `{{animation_speed}}`: Playback speed of the animation
  - `{{state_machines}}`: List of state machines for the artboard
  - For each state machine `{{#state_machines}}`:
    - `{{state_machine_name}}`: Name of the state machine
    - `{{state_machine_camel_case}}`: Name of the state machine in camelCase
    - `{{state_machine_pascal_case}}`: Name of the state machine in PascalCase
    - `{{state_machine_snake_case}}`: Name of the state machine in snake_case
    - `{{state_machine_kebab_case}}`: Name of the state machine in kebab-case
    - `{{inputs}}`: List of inputs for the state machine
    - For each input `{{#inputs}}`:
      - `{{input_name}}`: Name of the input
      - `{{input_camel_case}}` / `_pascal_case` / `_snake_case` / `_kebab_case`: Case-converted input name
      - `{{input_type}}`: Type of the input
      - `{{input_default_value}}`: Default value of the input
    - `{{states}}`: List of states for the state machine
    - For each state `{{#states}}`:
      - `{{state_name}}`: Name of the state
      - `{{state_camel_case}}` / `_pascal_case`: Case-converted state name
      - `{{state_type}}`: Type of the state
      - `{{transitions}}`: List of outgoing transitions, each with `{{transition_to}}` (target state)
  - `{{events}}`: List of events for the artboard
  - For each event `{{#events}}`:
    - `{{event_name}}`: Name of the event
    - `{{event_camel_case}}` / `_pascal_case` / `_snake_case` / `_kebab_case`: Case-converted event name
    - `{{event_type}}`: Type of the event (`{{is_general}}`, `{{is_open_url}}` flags)
    - `{{event_url}}`: URL for open-URL events
    - `{{event_target}}`: Target for open-URL events
    - `{{event_asset_id}}`: Associated asset ID, when present
  - `{{text_value_runs}}`: List of text value runs for the artboard
  - For each text value run `{{#text_value_runs}}`:
    - `{{text_value_run_name}}`: Name of the text value run
    - `{{text_value_run_camel_case}}`: Name of the text value run in camelCase
    - `{{text_value_run_pascal_case}}`: Name of the text value run in PascalCase
    - `{{text_value_run_snake_case}}`: Name of the text value run in snake_case
    - `{{text_value_run_kebab_case}}`: Name of the text value run in kebab-case
    - `{{text_value_run_default}}`: Default value of the text value run
    - `{{text_value_run_default_sanitized}}`: Default value with special characters encoded
    - `{{text_value_run_font_size}}`: Font size
    - `{{text_value_run_line_height}}`: Line height
    - `{{text_value_run_letter_spacing}}`: Letter spacing
    - `{{text_value_run_align}}`: Horizontal alignment
    - `{{text_value_run_vertical_align}}`: Vertical alignment
    - `{{text_value_run_sizing}}`: Sizing mode
    - `{{text_value_run_overflow}}`: Overflow behavior
    - `{{text_value_run_wrap}}`: Wrapping behavior
    - `{{text_value_run_font_asset_id}}`: Associated font asset ID
  - For each nested text value run `{{#nested_text_value_runs}}`:

    - `{{nested_text_value_run_name}}`: Name of the nested text value run
    - `{{nested_text_value_run_path}}`: Path of the nested text value run

  - `{{enums}}`: List of enums in the Rive file
  - For each enum `{{#enums}}`:

    - `{{enum_name}}`: Name of the enum
    - `{{enum_camel_case}}`: Name of the enum in camelCase
    - `{{enum_pascal_case}}`: Name of the enum in PascalCase
    - `{{enum_snake_case}}`: Name of the enum in snake_case
    - `{{enum_kebab_case}}`: Name of the enum in kebab-case
    - `{{enum_values}}`: List of enum values
    - For each enum value `{{#enum_values}}`:
      - `{{enum_value_key}}`: Key of the enum value
      - `{{enum_value_value}}`: Underlying integer value of the enum value
      - `{{enum_value_camel_case}}`: Key of the enum value in camelCase
      - `{{enum_value_pascal_case}}`: Key of the enum value in PascalCase
      - `{{enum_value_snake_case}}`: Key of the enum value in snake_case
      - `{{enum_value_kebab_case}}`: Key of the enum value in kebab-case

  - `{{view_models}}`: List of view models in the Rive file
  - For each view model `{{#view_models}}`:
    - `{{view_model_name}}`: Name of the view model
    - `{{view_model_camel_case}}`: Name of the view model in camelCase
    - `{{view_model_pascal_case}}`: Name of the view model in PascalCase
    - `{{view_model_snake_case}}`: Name of the view model in snake_case
    - `{{view_model_kebab_case}}`: Name of the view model in kebab-case
    - `{{instance_names}}`: List of named instances defined for the view model (sorted alphabetically)
    - For each instance `{{#instance_names}}`:
      - `{{instance_name}}`: Name of the instance
      - `{{instance_camel_case}}`: Name of the instance in camelCase
      - `{{instance_pascal_case}}`: Name of the instance in PascalCase
      - `{{last}}`: `true` for the final instance in the list (useful for separators)
    - `{{properties}}`: List of properties in the view model
    - For each property `{{#properties}}`:
      - `{{property_name}}`: Name of the property
      - `{{property_camel_case}}`: Name of the property in camelCase
      - `{{property_pascal_case}}`: Name of the property in PascalCase
      - `{{property_snake_case}}`: Name of the property in snake_case
      - `{{property_kebab_case}}`: Name of the property in kebab-case
      - `{{has_default_value}}`: Whether the property has a default value (read from the default instance)
      - `{{property_default_value}}`: Default value of the property, when available
      - `{{property_type}}`: Type information for the property
      - For property type `{{#property_type}}`:
        - `{{type_name}}`: Name of the type
        - `{{is_view_model}}`: Whether the property is a view model
        - `{{is_enum}}`: Whether the property is an enum
        - `{{is_string}}`: Whether the property is a string
        - `{{is_number}}`: Whether the property is a number
        - `{{is_integer}}`: Whether the property is an integer
        - `{{is_boolean}}`: Whether the property is a boolean
        - `{{is_color}}`: Whether the property is a color
        - `{{is_list}}`: Whether the property is a list
        - `{{is_trigger}}`: Whether the property is a trigger
        - `{{is_symbol_list_index}}`: Whether the property is a symbol list index
        - `{{is_asset_image}}`: Whether the property is an image asset reference
        - `{{backing_name}}`: Backing name of the property (view model / enum type)
        - `{{backing_camel_case}}`: Backing name in camelCase
        - `{{backing_pascal_case}}`: Backing name in PascalCase
        - `{{backing_snake_case}}`: Backing name in snake_case
        - `{{backing_kebab_case}}`: Backing name in kebab-case

**:warning: Warning:** For duplicated names (e.g., multiple artboards, animations, or assets with the same name), the original unique names will be preserved. However, the case-converted versions (such as camelCase, PascalCase, etc.) will have a unique identifier attached to avoid conflicts.

For example:

- Original names: "MyArtboard", "MyArtboard"
- Unique camelCase: "myArtboard", "myArtboardU1"

This ensures that all generated code and references remain unique and valid.

## Supported Languages

The tool ships with templates for **Dart**, **JSON**, **TypeScript** (VueRive declarations), and a **view model** output. More default exports will be added, and you can easily add your own by providing a custom template via the `-t` flag. The bundled templates live in the [`templates`](./templates) directory:

- [`dart_template.mustache`](./templates/dart_template.mustache) — type-safe Dart wrappers
- [`json_template.mustache`](./templates/json_template.mustache) — full JSON metadata dump
- [`ts_template.mustache`](./templates/ts_template.mustache) — TypeScript `VueRive` global declarations
- [`viewmodel_template.mustache`](./templates/viewmodel_template.mustache) — view model–focused output

## Contribute

Contributions are welcome! Please see our [Contributing Guidelines](CONTRIBUTING.md) for more details.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
