# Querying data in an Arf! document

## Parsing an Arf! document

```arf::parse``` operates on string data and returns the document tree:
```
std::string example_config = obtain_arf_data_from_somewhere();

auto result = arf::parse(example_config);    
if (result.has_value())
{
    // result->doc containts the parsed document
}
if (result.has_errors())
{
    // result.errors contains the errors emitted when parsing
}
```

## Querying key\value data

Key/value data is accessed by dot-separated path, where the last segment is assumed to be the key (the name of the value). 

```
// foo is declared in the root:
foo = 13

bar:
// baz is located under the category bar:
    baz = 42
```
* The path to foo is: ```foo```
* The path to baz is: ```bar.baz```

Accessor functions all take the document and a path as arguments and returns a std::optional<T>:
```
  std::optional<float> volume = arf::get_float(doc, "game_settings.audio.master_volume");
```
* ```arf::get_string``` -> ```std::string```
* ```arf::get_int``` -> ```int```
* ```arf::get_bool``` -> ```bool```
* ```arf::get_float``` -> ```float```

If the value at the path is of the requested type it is returned as such, otherwise a conversion is attempted. Failures result in ```std::nullopt```.

## Querying tabular data
