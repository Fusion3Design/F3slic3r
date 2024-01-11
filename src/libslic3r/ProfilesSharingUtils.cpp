///|/ Copyright (c) Prusa Research 2021 - 2023 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "ProfilesSharingUtils.hpp"
#include "Utils.hpp"
#include "format.hpp"
#include "PrintConfig.hpp"
#include "PresetBundle.hpp"
#include "Utils/DirectoriesUtils.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/log/trivial.hpp>

namespace Slic3r {

static bool load_preset_bundle_from_datadir(PresetBundle& preset_bundle)
{
    AppConfig app_config = AppConfig(AppConfig::EAppMode::Editor);
    if (!app_config.exists()) {
        BOOST_LOG_TRIVIAL(error) << "Configuration wasn't found. Check your 'datadir' value.";
        return false;
    }

    if (std::string error = app_config.load(); !error.empty()) {
        BOOST_LOG_TRIVIAL(error) << Slic3r::format("Error parsing PrusaSlicer config file, it is probably corrupted. "
            "Try to manually delete the file to recover from the error. Your user profiles will not be affected."
            "\n%1%\n%2%", app_config.config_path(), error);
        return false;
    }

    // just checking for existence of Slic3r::data_dir is not enough : it may be an empty directory
    // supplied as argument to --datadir; in that case we should still run the wizard
    preset_bundle.setup_directories();

    std::string delayed_error_load_presets;
    // Suppress the '- default -' presets.
    preset_bundle.set_default_suppressed(app_config.get_bool("no_defaults"));
    try {
        auto preset_substitutions = preset_bundle.load_presets(app_config, ForwardCompatibilitySubstitutionRule::EnableSystemSilent);
        if (!preset_substitutions.empty()) {
            BOOST_LOG_TRIVIAL(error) << "Some substitutions are found during loading presets.";
            return false;
        }

        // Post-process vendor map to delete non-installed models/varians

        VendorMap& vendors = preset_bundle.vendors;
        for (auto& [vendor_id, vendor_profile] : vendors) {
            std::vector<VendorProfile::PrinterModel> models;

            for (auto& printer_model : vendor_profile.models) {
                std::vector<VendorProfile::PrinterVariant> variants;

                for (const auto& variant : printer_model.variants) {
                    // check if printer model with variant is intalled
                    if (app_config.get_variant(vendor_id, printer_model.id, variant.name))
                        variants.push_back(variant);
                }

                if (!variants.empty()) {
                    if (printer_model.variants.size() != variants.size())
                        printer_model.variants = variants;
                    models.push_back(printer_model);
                }
            }

            if (!models.empty()) {
                if (vendor_profile.models.size() != models.size())
                    vendor_profile.models = models;
            }
        }
    }
    catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(error) << ex.what();
        return false;
    }

    return true;
}

namespace pt = boost::property_tree;
/*
struct PrinterAttr_
{
    std::string         model_name;
    std::string         variant;
};

static std::string get_printer_profiles(const VendorProfile* vendor_profile, 
                                        const PresetBundle* preset_bundle, 
                                        const PrinterAttr_& printer_attr)
{
    for (const auto& printer_model : vendor_profile->models) {
        if (printer_model.name != printer_attr.model_name)
            continue;

        for (const auto& variant : printer_model.variants)
            if (variant.name == printer_attr.variant)
            {
                pt::ptree data_node;
                data_node.put("printer_model", printer_model.name);
                data_node.put("printer_variant", printer_attr.variant);

                pt::ptree printer_profiles_node;
                for (const Preset& printer_preset : preset_bundle->printers) {
                    if (printer_preset.vendor->id == vendor_profile->id &&
                        printer_preset.is_visible && // ???
                        printer_preset.config.opt_string("printer_model") == printer_model.id &&
                        printer_preset.config.opt_string("printer_variant") == printer_attr.variant) {
                        pt::ptree profile_node;
                        profile_node.put("", printer_preset.name);
                        printer_profiles_node.push_back(std::make_pair("", profile_node));
                    }
                }
                data_node.add_child("printer_profiles", printer_profiles_node);

                // Serialize the tree into JSON and return it.
                std::stringstream ss;
                pt::write_json(ss, data_node);
                return ss.str();
            }
    }

    return "";
}

std::string get_json_printer_profiles(const std::string& printer_model_name, const std::string& printer_variant)
{
    if (!is_datadir())
        return "";

    PrinterAttr_ printer_attr({printer_model_name, printer_variant});

    PresetBundle preset_bundle;
    if (!load_preset_bundle_from_datadir(preset_bundle))
        return "";

    const VendorMap& vendors = preset_bundle.vendors;
    for (const auto& [vendor_id, vendor] : vendors) {
        std::string out = get_printer_profiles(&vendor, &preset_bundle, printer_attr);
        if (!out.empty())
            return out;
    }

    return "";
}
*/

struct PrinterAttr
{
    std::string vendor_id;
    std::string model_id;
    std::string variant_name;
};

static bool is_compatible_preset(const Preset& printer_preset, const PrinterAttr& attr)
{
    return  printer_preset.vendor->id == attr.vendor_id &&
            printer_preset.config.opt_string("printer_model") == attr.model_id &&
            printer_preset.config.opt_string("printer_variant") == attr.variant_name;
}

static void add_profile_node(pt::ptree& printer_profiles_node, const std::string& preset_name)
{
    pt::ptree profile_node;
    profile_node.put("", preset_name);
    printer_profiles_node.push_back(std::make_pair("", profile_node));
}

static void get_printer_profiles_node(pt::ptree& printer_profiles_node, 
                                      pt::ptree& user_printer_profiles_node,
                                      const PrinterPresetCollection& printer_presets,
                                      const PrinterAttr& attr)
{
    printer_profiles_node.clear();
    user_printer_profiles_node.clear();

    for (const Preset& printer_preset : printer_presets) {

        if (printer_preset.is_user()) {
            const Preset* parent_preset = printer_presets.get_preset_parent(printer_preset);
            if (parent_preset && printer_preset.is_visible && is_compatible_preset(*parent_preset, attr))
                add_profile_node(user_printer_profiles_node, printer_preset.name);
        }
        else if (printer_preset.is_visible && is_compatible_preset(printer_preset, attr))
            add_profile_node(printer_profiles_node, printer_preset.name);
    }
}

static void add_printer_models(pt::ptree& vendor_node,
                               const VendorProfile* vendor_profile,
                               PrinterTechnology printer_technology,
                               const PrinterPresetCollection& printer_presets)
{
    for (const auto& printer_model : vendor_profile->models) {
        if (printer_technology != ptAny && printer_model.technology != printer_technology)
            continue;

        pt::ptree variants_node;
        pt::ptree printer_profiles_node;
        pt::ptree user_printer_profiles_node;

        if (printer_model.technology == ptSLA) {
            PrinterAttr attr({ vendor_profile->id, printer_model.id, "default" });

            get_printer_profiles_node(printer_profiles_node, user_printer_profiles_node, printer_presets, attr);
            if (printer_profiles_node.empty() && user_printer_profiles_node.empty())
                continue;
        }
        else {
            for (const auto& variant : printer_model.variants) {

                PrinterAttr attr({ vendor_profile->id, printer_model.id, variant.name });

                get_printer_profiles_node(printer_profiles_node, user_printer_profiles_node, printer_presets, attr);
                if (printer_profiles_node.empty() && user_printer_profiles_node.empty())
                    continue;

                pt::ptree variant_node;
                variant_node.put("name", variant.name);
                variant_node.add_child("printer_profiles", printer_profiles_node);
                if (!user_printer_profiles_node.empty())
                    variant_node.add_child("user_printer_profiles", user_printer_profiles_node);

                variants_node.push_back(std::make_pair("", variant_node));
            }

            if (variants_node.empty())
                continue;
        }

        pt::ptree data_node;
        data_node.put("id", printer_model.id);
        data_node.put("name", printer_model.name);
        data_node.put("technology", printer_model.technology == ptFFF ? "FFF" : "SLA");

        if (!variants_node.empty())
            data_node.add_child("variants", variants_node);
        else {
            data_node.add_child("printer_profiles", printer_profiles_node);
            if (!user_printer_profiles_node.empty())
                data_node.add_child("user_printer_profiles", user_printer_profiles_node);
        }

        data_node.put("vendor_name", vendor_profile->name);
        data_node.put("vendor_id", vendor_profile->id);

        vendor_node.push_back(std::make_pair("", data_node));
    }
}

std::string get_json_printer_models(PrinterTechnology printer_technology)
{
    PresetBundle preset_bundle;
    if (!load_preset_bundle_from_datadir(preset_bundle))
        return "";
            
    pt::ptree vendor_node;

    const VendorMap& vendors_map = preset_bundle.vendors;
    for (const auto& [vendor_id, vendor] : vendors_map)
        add_printer_models(vendor_node, &vendor, printer_technology, preset_bundle.printers);

    pt::ptree root;
    root.add_child("printer_models", vendor_node);

    // Serialize the tree into JSON and return it.
    std::stringstream ss;
    pt::write_json(ss, root);
    return ss.str();
}

static std::string get_installed_print_and_filament_profiles(const PresetBundle* preset_bundle, const Preset* printer_preset)
{
    PrinterTechnology printer_technology = printer_preset->printer_technology();

    pt::ptree print_profiles;
    pt::ptree user_print_profiles;

    const PresetWithVendorProfile printer_preset_with_vendor_profile = preset_bundle->printers.get_preset_with_vendor_profile(*printer_preset);

    const PresetCollection& print_presets       = printer_technology == ptFFF ? preset_bundle->prints    : preset_bundle->sla_prints;
    const PresetCollection& material_presets    = printer_technology == ptFFF ? preset_bundle->filaments : preset_bundle->sla_materials;
    const std::string       material_node_name  = printer_technology == ptFFF ? "filament_profiles"      : "sla_material_profiles";

    for (auto print_preset : print_presets) {

        const PresetWithVendorProfile print_preset_with_vendor_profile = print_presets.get_preset_with_vendor_profile(print_preset);

        if (is_compatible_with_printer(print_preset_with_vendor_profile, printer_preset_with_vendor_profile))
        {
            pt::ptree materials_profile_node;
            pt::ptree user_materials_profile_node;

            for (auto material_preset : material_presets) {

                // ?! check visible and no-template presets only
                if (!material_preset.is_visible || (material_preset.vendor && material_preset.vendor->templates_profile))
                    continue;

                const PresetWithVendorProfile material_preset_with_vendor_profile = material_presets.get_preset_with_vendor_profile(material_preset);

                if (is_compatible_with_printer(material_preset_with_vendor_profile, printer_preset_with_vendor_profile) &&
                    is_compatible_with_print(material_preset_with_vendor_profile, print_preset_with_vendor_profile, printer_preset_with_vendor_profile)) {
                    pt::ptree material_node;
                    material_node.put("", material_preset.name);
                    if (material_preset.is_user())
                        user_materials_profile_node.push_back(std::make_pair("", material_node));
                    else
                        materials_profile_node.push_back(std::make_pair("", material_node));
                }
            }

            pt::ptree print_profile_node;
            print_profile_node.put("name", print_preset.name);
            print_profile_node.add_child(material_node_name, materials_profile_node);
            if (!user_materials_profile_node.empty())
                print_profile_node.add_child("user_" + material_node_name, user_materials_profile_node);

            if (print_preset.is_user())
                user_print_profiles.push_back(std::make_pair("", print_profile_node));
            else
                print_profiles.push_back(std::make_pair("", print_profile_node));
        }
    }

    if (print_profiles.empty() && user_print_profiles.empty())
        return "";

    pt::ptree tree;
    tree.put("printer_profile", printer_preset->name);
    tree.add_child("print_profiles", print_profiles);
    if (!user_print_profiles.empty())
        tree.add_child("user_print_profiles", user_print_profiles);

    // Serialize the tree into JSON and return it.
    std::stringstream ss;
    pt::write_json(ss, tree);
    return ss.str();
}

std::string get_json_print_filament_profiles(const std::string& printer_profile)
{
    PresetBundle preset_bundle;
    if (load_preset_bundle_from_datadir(preset_bundle)) {
        const Preset* preset = preset_bundle.printers.find_preset(printer_profile, false, false);
        if (preset)
            return get_installed_print_and_filament_profiles(&preset_bundle, preset);
    }

    return "";
}

// Helper function for FS
bool load_full_print_config(const std::string& print_preset_name, const std::string& filament_preset_name, const std::string& printer_preset_name, DynamicPrintConfig& config)
{
    PresetBundle preset_bundle;
    if (!load_preset_bundle_from_datadir(preset_bundle)){
        BOOST_LOG_TRIVIAL(error) << Slic3r::format("Failed to load data from the datadir '%1%'.", data_dir());
        return false;
    }

    config = {};
    config.apply(FullPrintConfig::defaults());

    bool is_failed{ false };

    if (const Preset* print_preset = preset_bundle.prints.find_preset(print_preset_name))
        config.apply_only(print_preset->config, print_preset->config.keys());
    else {
        BOOST_LOG_TRIVIAL(warning) << Slic3r::format("Print profile '%1%' wasn't found.", print_preset_name);
        is_failed |= true;
    }

    if (const Preset* filament_preset = preset_bundle.filaments.find_preset(filament_preset_name))
        config.apply_only(filament_preset->config, filament_preset->config.keys());
    else {
        BOOST_LOG_TRIVIAL(warning) << Slic3r::format("Filament profile '%1%' wasn't found.", filament_preset_name);
        is_failed |= true;
    }

    if (const Preset* printer_preset = preset_bundle.printers.find_preset(printer_preset_name))
        config.apply_only(printer_preset->config, printer_preset->config.keys());
    else {
        BOOST_LOG_TRIVIAL(warning) << Slic3r::format("Printer profile '%1%' wasn't found.", printer_preset_name);
        is_failed |= true;
    }

    return !is_failed;
}

} // namespace Slic3r
