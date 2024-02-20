///|/ Copyright (c) Prusa Research 2021 - 2023 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "ProfilesSharingUtils.hpp"
#include "libslic3r/utils.hpp"

#include "slic3r/GUI/ConfigWizard_private.hpp"
#include "slic3r/GUI/format.hpp"

#include <boost/property_tree/json_parser.hpp>

namespace Slic3r {

namespace pt = boost::property_tree;

using namespace GUI;

static pt::ptree get_printer_models_for_vendor(const VendorProfile* vendor_profile, PrinterTechnology printer_technology)
{
    pt::ptree vendor_node;

    for (const auto& printer_model : vendor_profile->models) {
        if (printer_technology != ptAny && printer_model.technology != printer_technology)
            continue;

        pt::ptree data_node;
        data_node.put("vendor_name", vendor_profile->name);
        data_node.put("id", printer_model.id);
        data_node.put("name", printer_model.name);

        if (printer_model.technology == ptFFF) {
            pt::ptree variants_node;
            for (const auto& variant : printer_model.variants) {
                pt::ptree variant_node;
                variant_node.put("", variant.name);
                variants_node.push_back(std::make_pair("", variant_node));
            }
            data_node.add_child("variants", variants_node);
        }

        data_node.put("technology", printer_model.technology == ptFFF ? "FFF" : "SLA");
        data_node.put("family", printer_model.family);

        pt::ptree def_materials_node;
        for (const std::string& material : printer_model.default_materials) {
            pt::ptree material_node;
            material_node.put("", material);
            def_materials_node.push_back(std::make_pair("", material_node));
        }
        data_node.add_child("default_materials", def_materials_node);

        vendor_node.push_back(std::make_pair("", data_node));
    }

    return vendor_node;
}

static bool load_preset_bandle_from_datadir(PresetBundle& preset_bundle)
{
    AppConfig app_config = AppConfig(AppConfig::EAppMode::Editor);
    if (!app_config.exists()) {
        printf("Configuration wasn't found. Check your 'datadir' value.\n");
        return false;
    }

    printf("Loading: AppConfig");

    if (std::string error = app_config.load(); !error.empty()) {
        throw Slic3r::RuntimeError(Slic3r::format("Error parsing PrusaSlicer config file, it is probably corrupted. "
            "Try to manually delete the file to recover from the error. Your user profiles will not be affected."
            "\n\n%1%\n\n%2%", app_config.config_path(), error));
        return false;
    }

    // just checking for existence of Slic3r::data_dir is not enough : it may be an empty directory
    // supplied as argument to --datadir; in that case we should still run the wizard
    preset_bundle.setup_directories();

    std::string delayed_error_load_presets;
    // Suppress the '- default -' presets.
    preset_bundle.set_default_suppressed(app_config.get_bool("no_defaults"));
    try {
        printf(", presets");
        auto preset_substitutions = preset_bundle.load_presets(app_config, ForwardCompatibilitySubstitutionRule::EnableSystemSilent);
        if (!preset_substitutions.empty()) {
            printf("Some substitutions are found during loading presets.\n");
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

        return true;
    }
    catch (const std::exception& ex) {
        delayed_error_load_presets = ex.what();
        printf(delayed_error_load_presets.c_str());
        return false;
    }
}

std::string get_json_printer_models(PrinterTechnology printer_technology)
{
    // Build a property tree with all the information.
    pt::ptree root;

    if (data_dir().empty()) {
        printf("Loading of all known vendors .");
        BundleMap bundles = BundleMap::load();

        for (const auto& [bundle_id, bundle] : bundles) {
            pt::ptree vendor_node = get_printer_models_for_vendor(bundle.vendor_profile, printer_technology);
            if (!vendor_node.empty())
                root.add_child(bundle_id, vendor_node);
        }
    }
    else {
        PresetBundle preset_bundle;
        if (!load_preset_bandle_from_datadir(preset_bundle))
            return "";
        printf(", vendors");
            
        const VendorMap& vendors_map = preset_bundle.vendors;
        for (const auto& [vendor_id, vendor] : vendors_map) {
            pt::ptree vendor_node = get_printer_models_for_vendor(&vendor, printer_technology);
            if (!vendor_node.empty())
                root.add_child(vendor_id, vendor_node);
        }
    }

    // Serialize the tree into JSON and return it.
    std::stringstream ss;
    pt::write_json(ss, root);
    return ss.str();
}



struct PrinterAttr
{
    std::string         model_name;
    std::string         variant;
};

static std::string get_printer_profiles(const VendorProfile* vendor_profile, 
                                        const PresetBundle* preset_bundle, 
                                        const PrinterAttr& printer_attr)
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
    PrinterAttr printer_attr({printer_model_name, printer_variant});

    if (data_dir().empty()) {
        printf("Loading of all known vendors .");
        BundleMap bundles = BundleMap::load();

        for (const auto& [bundle_id, bundle] : bundles) {
            std::string out = get_printer_profiles(bundle.vendor_profile, bundle.preset_bundle.get(), printer_attr);
            if (!out.empty())
                return out;
        }
    }
    else {
        PresetBundle preset_bundle;
        if (!load_preset_bandle_from_datadir(preset_bundle))
            return "";
        printf(", vendors");

        const VendorMap& vendors = preset_bundle.vendors;
        for (const auto& [vendor_id, vendor] : vendors) {
            std::string out = get_printer_profiles(&vendor, &preset_bundle, printer_attr);
            if (!out.empty())
                return out;
        }
    }

    return "";
}


static std::string get_installed_print_and_filament_profiles(const PresetBundle* preset_bundle, const Preset* printer_preset)
{
    PrinterTechnology printer_technology = printer_preset->printer_technology();

    printf("\n\nSearching for compatible print profiles .");

    pt::ptree print_profiles;

    const PresetWithVendorProfile printer_preset_with_vendor_profile = preset_bundle->printers.get_preset_with_vendor_profile(*printer_preset);

    const PresetCollection& print_presets       = printer_technology == ptFFF ? preset_bundle->prints    : preset_bundle->sla_prints;
    const PresetCollection& material_presets    = printer_technology == ptFFF ? preset_bundle->filaments : preset_bundle->sla_materials;
    const std::string       material_node_name  = printer_technology == ptFFF ? "filament_profiles"      : "sla_material_profiles";

    int output_counter{ 0 };
    for (auto print_preset : print_presets) {

        ++output_counter;
        if (output_counter == 10) {
            printf(".");
            output_counter = 0;
        }

        const PresetWithVendorProfile print_preset_with_vendor_profile = print_presets.get_preset_with_vendor_profile(print_preset);

        if (is_compatible_with_printer(print_preset_with_vendor_profile, printer_preset_with_vendor_profile))
        {
            pt::ptree print_profile_node;
            print_profile_node.put("name", print_preset.name);

            pt::ptree materials_profile_node;

            for (auto material_preset : material_presets) {

                // ?! check visible and no-template presets only
                if (!material_preset.is_visible || material_preset.vendor->templates_profile)
                    continue;

                const PresetWithVendorProfile material_preset_with_vendor_profile = material_presets.get_preset_with_vendor_profile(material_preset);

                if (is_compatible_with_printer(material_preset_with_vendor_profile, printer_preset_with_vendor_profile) &&
                    is_compatible_with_print(material_preset_with_vendor_profile, print_preset_with_vendor_profile, printer_preset_with_vendor_profile)) {
                    pt::ptree material_node;
                    material_node.put("", material_preset.name);
                    materials_profile_node.push_back(std::make_pair("", material_node));
                }
            }

            print_profile_node.add_child(material_node_name, materials_profile_node);
            print_profiles.push_back(std::make_pair("", print_profile_node));
        }
    }

    printf("\n");

    if (print_profiles.empty())
        return "";

    pt::ptree tree;
    tree.put("printer_profile", printer_preset->name);
    tree.add_child("print_profiles", print_profiles);

    // Serialize the tree into JSON and return it.
    std::stringstream ss;
    pt::write_json(ss, tree);
    return ss.str();
}

std::string get_json_print_filament_profiles(const std::string& printer_profile)
{
    if (data_dir().empty()) {
        printf("Loading of all known vendors .");
        BundleMap bundles = BundleMap::load();

        printf(GUI::format("\n\nSearching for \"%1%\" .", printer_profile).c_str());

        for (const auto& [bundle_id, bundle] : bundles) {
            printf(".");
            if (const Preset* preset = bundle.preset_bundle->printers.find_preset(printer_profile, false, false))
                return get_installed_print_and_filament_profiles(bundle.preset_bundle.get(), preset);
        }
    }
    else {
        PresetBundle preset_bundle;
        if (!load_preset_bandle_from_datadir(preset_bundle))
            return "";

        if (const Preset* preset = preset_bundle.printers.find_preset(printer_profile, false, false))
            return get_installed_print_and_filament_profiles(&preset_bundle, preset);
    }

    return "";
}

} // namespace Slic3r
