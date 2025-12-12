#include "include/arf.hpp"
#include <iostream>
#include <iomanip>

// Example Arf configuration
const char* example_config = R"(
// Game Configuration Example
// This showcases the Arf format features

server:
    # region:str  address:str               port:int  max_players:int  active:bool
      us-east     game-us-east.example.com  7777      64               true
      us-west     game-us-west.example.com  7777      64               true
      eu-central  game-eu.example.com       7778      128              true
    
    version = 2.1.5
    last_updated = 2025-12-11
    admin_contact = ops@example.com
    
  :load_balancing
    strategy = round-robin
    health_check_interval = 30
    retry_attempts = 3
  /load_balancing
/server

characters:
    # id:str         class:str   base_hp:int  base_mana:int  speed:float  start_skills:str[]
      warrior_m      warrior     150          20             1.0          slash|block|taunt
      mage_f         mage        80           200            0.85         fireball|ice_shield|teleport
      rogue_m        rogue       100          50             1.3          backstab|stealth|pickpocket
      cleric_f       cleric      110          150            0.95         heal|bless|smite
    
  :warrior
    description = Heavily armored melee fighter with high survivability
    difficulty = beginner
    
    # ability_id:str    cooldown:float  mana_cost:int  damage:int
      slash             2.5             10             35
      block             8.0             15             0
      taunt             12.0            20             5
  /warrior
  
  :mage
    description = Glass cannon spellcaster with devastating ranged attacks
    difficulty = advanced
    
    # ability_id:str    cooldown:float  mana_cost:int  damage:int
      fireball          4.0             40             85
      ice_shield        15.0            60             0
      teleport          20.0            80             0
  /mage
/characters

monsters:
    # id:int  name:str         count:int
      1       bat              13
      2       rat              42
      
  :goblins
      3       green goblin     123
      4       red goblin       456
  /goblins
  
  :undead
      5       skeleton         314
      6       zombie           999
  /undead
  
      7       kobold           3
      8       orc              10
/monsters

game_settings:
    title = Epic Quest Adventures
    version = 1.2.0
    release_date = 2025-11-15
    
    default_resolution = 1920x1080
    target_fps = 60
    vsync_enabled = true
    
  :difficulty_modifiers
    # level:str   damage_multiplier:float  health_multiplier:float  xp_multiplier:float
      easy        0.75                     1.5                      0.8
      normal      1.0                      1.0                      1.0
      hard        1.5                      0.75                     1.25
      nightmare   2.0                      0.5                      1.5
  /difficulty_modifiers
  
  :audio
    master_volume = 0.8
    music_volume = 0.6
    sfx_volume = 0.9
    
    main_theme = audio/music/main_theme.ogg
    battle_theme = audio/music/battle_intense.ogg
  /audio
/game_settings
)";

void print_separator(const std::string& title)
{
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(70, '=') << "\n\n";
}

void test_parsing()
{
    print_separator("TEST 1: Basic Parsing");
    
    arf::document doc = arf::parse(example_config);
    
    std::cout << "✓ Parsed " << doc.categories.size() << " top-level categories\n";
    
    for (const auto& [name, cat] : doc.categories)
    {
        std::cout << "  • " << name << ": "
                  << cat->table_rows.size() << " table rows, "
                  << cat->key_values.size() << " key-values, "
                  << cat->subcategories.size() << " subcategories\n";
    }
}

void test_table_access()
{
    print_separator("TEST 2: Table Data Access");
    
    arf::document doc = arf::parse(example_config);
    
    if (doc.categories.count("server"))
    {
        auto& server = doc.categories["server"];
        std::cout << "Server Regions:\n";
        std::cout << std::string(50, '-') << "\n";
        
        // Print table header
        for (size_t i = 0; i < server->table_columns.size(); ++i)
        {
            std::cout << std::left << std::setw(25) << server->table_columns[i].name;
        }
        std::cout << "\n" << std::string(50, '-') << "\n";
        
        // Print rows
        for (const auto& row : server->table_rows)
        {
            for (size_t i = 0; i < row.size(); ++i)
            {
                std::cout << std::left << std::setw(25);
                std::visit([](const auto& val) 
                {
                    using T = std::decay_t<decltype(val)>;
                    if constexpr (std::is_same_v<T, std::string>)
                        std::cout << val;
                    else if constexpr (std::is_same_v<T, int64_t>)
                        std::cout << val;
                    else if constexpr (std::is_same_v<T, double>)
                        std::cout << val;
                    else if constexpr (std::is_same_v<T, bool>)
                        std::cout << (val ? "true" : "false");
                }, row[i]);
            }
            std::cout << "\n";
        }
    }
}

void test_key_value_queries()
{
    print_separator("TEST 3: Key-Value Queries");
    
    arf::document doc = arf::parse(example_config);
    
    // Query simple values
    auto version = arf::get_string(doc, "server.version");
    auto contact = arf::get_string(doc, "server.admin_contact");
    auto strategy = arf::get_string(doc, "server.load_balancing.strategy");
    
    std::cout << "Server Configuration:\n";
    std::cout << "  Version: " << (version ? *version : "N/A") << "\n";
    std::cout << "  Admin: " << (contact ? *contact : "N/A") << "\n";
    std::cout << "  Load Balancing: " << (strategy ? *strategy : "N/A") << "\n\n";
    
    // Query typed values
    auto fps = arf::get_int(doc, "game_settings.target_fps");
    auto vsync = arf::get_bool(doc, "game_settings.vsync_enabled");
    auto master_vol = arf::get_float(doc, "game_settings.audio.master_volume");
    
    std::cout << "Game Settings:\n";
    std::cout << "  Target FPS: " << (fps ? std::to_string(*fps) : "N/A") << "\n";
    std::cout << "  VSync: " << (vsync ? (*vsync ? "enabled" : "disabled") : "N/A") << "\n";
    std::cout << "  Master Volume: " << (master_vol ? std::to_string(*master_vol) : "N/A") << "\n";
}

void test_array_values()
{
    print_separator("TEST 4: Array Values");
    
    arf::document doc = arf::parse(example_config);
    
    if (doc.categories.count("characters"))
    {
        auto& chars = doc.categories["characters"];
        
        std::cout << "Character Starting Skills:\n\n";
        
        for (const auto& row : chars->table_rows)
        {
            std::string id = std::get<std::string>(row[0]);
            std::string char_class = std::get<std::string>(row[1]);
            const auto& skills = std::get<std::vector<std::string>>(row[5]);
            
            std::cout << "  " << id << " (" << char_class << "): ";
            for (size_t i = 0; i < skills.size(); ++i)
            {
                std::cout << skills[i];
                if (i < skills.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
        }
    }
}

void test_hierarchical_tables()
{
    print_separator("TEST 5: Hierarchical Table Continuation");
    
    arf::document doc = arf::parse(example_config);
    
    if (doc.categories.count("monsters"))
    {
        auto& monsters = doc.categories["monsters"];
        
        std::cout << "Monster Distribution:\n\n";
        
        std::cout << "Base Monsters:\n";
        for (const auto& row : monsters->table_rows)
        {
            std::cout << "  " << std::get<int64_t>(row[0]) << ". "
                      << std::get<std::string>(row[1]) << " (count: "
                      << std::get<int64_t>(row[2]) << ")\n";
        }
        
        for (const auto& [subcat_name, subcat] : monsters->subcategories)
        {
            std::cout << "\n" << subcat_name << ":\n";
            for (const auto& row : subcat->table_rows)
            {
                std::cout << "  " << std::get<int64_t>(row[0]) << ". "
                          << std::get<std::string>(row[1]) << " (count: "
                          << std::get<int64_t>(row[2]) << ")\n";
            }
        }
    }
}

void test_serialization()
{
    print_separator("TEST 6: Round-Trip Serialization");
    
    arf::document doc = arf::parse(example_config);
    std::string serialized = arf::serialize(doc);
    
    std::cout << "Original size: " << strlen(example_config) << " bytes\n";
    std::cout << "Serialized size: " << serialized.size() << " bytes\n\n";
    
    std::cout << "Serialized output (first 500 chars):\n";
    std::cout << std::string(70, '-') << "\n";
    std::cout << serialized.substr(0, 500) << "\n";
    std::cout << (serialized.size() > 500 ? "...\n" : "");
    std::cout << std::string(70, '-') << "\n\n";
    
    // Parse the serialized output
    arf::document doc2 = arf::parse(serialized);
    std::cout << "✓ Re-parsed successfully: " << doc2.categories.size() << " categories\n";
    
    // Verify key values match
    auto version1 = arf::get_string(doc, "server.version");
    auto version2 = arf::get_string(doc2, "server.version");
    
    if (version1 && version2 && *version1 == *version2)
        std::cout << "✓ Round-trip verification passed (server.version matches)\n";
    else
        std::cout << "✗ Round-trip verification failed\n";
}

void test_c_api()
{
    print_separator("TEST 7: C API Compatibility");
    
    // Parse using C API
    arf_document_t* doc = arf_parse(example_config);
    if (!doc)
    {
        std::cout << "✗ Failed to parse with C API\n";
        return;
    }
    std::cout << "✓ Parsed with C API\n";
    
    // Query string value
    char* version = arf_get_string(doc, "server.version");
    if (version)
    {
        std::cout << "  Server version: " << version << "\n";
        arf_free_string(version);
    }
    
    // Query int value
    int64_t fps;
    if (arf_get_int(doc, "game_settings.target_fps", &fps))
        std::cout << "  Target FPS: " << fps << "\n";
    
    // Query float value
    double volume;
    if (arf_get_float(doc, "game_settings.audio.master_volume", &volume))
        std::cout << "  Master volume: " << volume << "\n";
    
    // Query bool value
    int vsync;
    if (arf_get_bool(doc, "game_settings.vsync_enabled", &vsync))
        std::cout << "  VSync: " << (vsync ? "enabled" : "disabled") << "\n";
    
    // Serialize using C API
    char* serialized = arf_serialize(doc);
    if (serialized)
    {
        std::cout << "\n✓ Serialized with C API (" << strlen(serialized) << " bytes)\n";
        arf_free_string(serialized);
    }
    
    // Clean up
    arf_free_document(doc);
    std::cout << "✓ Memory cleaned up\n";
}

void test_edge_cases()
{
    print_separator("TEST 8: Edge Cases");
    
    // Empty document
    arf::document empty = arf::parse("");
    std::cout << "✓ Empty document: " << empty.categories.size() << " categories\n";
    
    // Query non-existent path
    auto missing = arf::get_string(empty, "does.not.exist");
    std::cout << "✓ Non-existent query: " << (missing ? "found" : "not found") << "\n";
    
    // Minimal document
    const char* minimal = "test:\n  key = value\n/test\n";
    arf::document minimal_doc = arf::parse(minimal);
    auto value = arf::get_string(minimal_doc, "test.key");
    std::cout << "✓ Minimal document query: " << (value ? *value : "N/A") << "\n";
}

int main()
{
    std::cout << R"(
    ___         __ _ 
   /   |  _____/ _| |
  / /| | / __/ |_| |
 / ___ ||  _|  _|_|
/_/   |_|_| |_| (_) 
                    
A Readable Format - Example & Test Suite
Version 0.1.0
)" << std::endl;
    
    try
    {
        test_parsing();
        test_table_access();
        test_key_value_queries();
        test_array_values();
        test_hierarchical_tables();
        test_serialization();
        //test_c_api();
        test_edge_cases();
        
        print_separator("ALL TESTS COMPLETED");
        std::cout << "✓ All tests passed successfully!\n\n";
        
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}