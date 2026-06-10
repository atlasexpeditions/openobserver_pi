#include "ooProjectFieldMapper.h"

static wxString NormalizeColumnName(wxString value)
{
    value.MakeLower();
    value.Trim(true);
    value.Trim(false);

    value.Replace("_", " ");
    value.Replace("-", " ");
    value.Replace(".", " ");
    value.Replace("/", " ");

    while (value.Contains("  ")) {
        value.Replace("  ", " ");
    }

    return value;
}

static bool HasFieldType(const wxArrayString& availableFieldTypes, const wxString& fieldType)
{
    return availableFieldTypes.Index(fieldType) != wxNOT_FOUND;
}

static wxString ListingOrText(const wxArrayString& availableFieldTypes, const wxString& listingName)
{
    if (HasFieldType(availableFieldTypes, listingName)) {
        return listingName;
    }

    return "Text";
}

static wxString FirstAvailableListing(
    const wxArrayString& availableFieldTypes,
    const wxString& preferredListing,
    const wxString& fallbackListing)
{
    if (HasFieldType(availableFieldTypes, preferredListing)) {
        return preferredListing;
    }

    if (HasFieldType(availableFieldTypes, fallbackListing)) {
        return fallbackListing;
    }

    return "Text";
}

wxString ooProjectFieldMapper::SuggestFieldTypeForColumn(
    const wxString& columnName,
    const wxArrayString& availableFieldTypes)
{
    const wxString name = NormalizeColumnName(columnName);

    // If a CSV header already matches a known Open Observer field type or listing,
    // keep that exact type. This lets advanced users prepare explicit templates.
    for (size_t i = 0; i < availableFieldTypes.GetCount(); ++i) {
        const wxString fieldType = availableFieldTypes[i];

        if (fieldType.IsEmpty() ||
            fieldType.EndsWith(":") ||
            NormalizeColumnName(fieldType).IsSameAs(" ")) {
            continue;
        }

        if (NormalizeColumnName(fieldType).IsSameAs(name)) {
            return fieldType;
        }
    }

    // Internal bridge to OpenCPN marks. Create-from-table ignores this later;
    // the internal field is added silently at the end of the generated project.
    if (name == "mark guid" ||
        name == "mark id" ||
        name == "opencpn mark guid") {
        return "Mark GUID";
    }

    if ((name.Contains("nmea") && name.Contains("record")) ||
        name == "nmea file" ||
        name == "nmea recording") {
        return "NMEA Recording";
    }

    // Date, time and position.
    if (name.Contains("end") && name.Contains("timestamp")) return "End Timestamp UTC";
    if (name.Contains("start") && name.Contains("timestamp")) return "Start Timestamp UTC";
    if (name == "timestamp" ||
        name == "datetime" ||
        name == "date time" ||
        name == "utc timestamp" ||
        name.Contains("timestamp utc")) {
        return "Start Timestamp UTC";
    }

    if (name.Contains("end") && name.Contains("latitude")) return "End Latitude";
    if (name.Contains("end") && name.Contains("longitude")) return "End Longitude";
    if (name.Contains("start") && name.Contains("latitude")) return "Start Latitude";
    if (name.Contains("start") && name.Contains("longitude")) return "Start Longitude";

    if (name == "lat" ||
        name == "latitude" ||
        name == "start lat" ||
        name == "start latitude" ||
        name == "gps lat" ||
        name == "gps latitude" ||
        name.Contains(" latitude") ||
        name.Contains(" lat ") ||
        name == "y") {
        return "Start Latitude";
    }

    if (name == "lon" ||
        name == "long" ||
        name == "lng" ||
        name == "longitude" ||
        name == "start lon" ||
        name == "start long" ||
        name == "start longitude" ||
        name == "gps lon" ||
        name == "gps long" ||
        name == "gps longitude" ||
        name.Contains(" longitude") ||
        name.Contains(" lon ") ||
        name.Contains(" lng ") ||
        name == "x") {
        return "Start Longitude";
    }

    if (name.Contains("end") && name.Contains("date")) return "End Date";
    if (name.Contains("end") && name.Contains("time")) return "End Time";
    if (name == "date" || name.Contains("start date")) return "Start Date";
    if (name == "time" || name.Contains("start time")) return "Start Time";

    if (name == "duration" ||
        name.Contains("observation duration")) {
        return "Observation Duration";
    }

    if (name == "distance") return "Distance";

    if (name == "obs id" ||
        name == "observation id" ||
        name == "record id" ||
        name == "event id") {
        return "Observation ID";
    }

    // Common citizen-science / protocol fields.
    if (name.Contains("species") ||
        name.Contains("taxon") ||
        name.Contains("scientific name") ||
        name.Contains("common name") ||
        name.Contains("espece") ||
        name.Contains("espèce")) {
        return FirstAvailableListing(
            availableFieldTypes,
            "Atlas oceanic species",
            "Ocean species");
    }

    if (name.Contains("behavior") ||
        name.Contains("behaviour") ||
        name.Contains("comportement")) {
        return ListingOrText(availableFieldTypes, "Animal behavior");
    }

    if (name.Contains("animal count") ||
        name == "count" ||
        name == "number" ||
        name == "nb" ||
        name.Contains("nombre") ||
        name.Contains("group size") ||
        name.Contains("pod size")) {
        return ListingOrText(availableFieldTypes, "Animal count");
    }

    if (name.Contains("animal distance") ||
        name.Contains("distance animal") ||
        name.Contains("distance to animal")) {
        return ListingOrText(availableFieldTypes, "Animal distance");
    }

    if (name.Contains("effort")) return ListingOrText(availableFieldTypes, "Effort");
    if (name.Contains("engine") || name.Contains("moteur")) return ListingOrText(availableFieldTypes, "Engine");
    if (name.Contains("visibility") || name.Contains("visibilite") || name.Contains("visibilité")) return ListingOrText(availableFieldTypes, "Visibility");
    if (name.Contains("glare")) return ListingOrText(availableFieldTypes, "Glare");
    if (name.Contains("cloud")) return ListingOrText(availableFieldTypes, "Cloud cover");
    if (name.Contains("beaufort") || name.Contains("bft")) return ListingOrText(availableFieldTypes, "BFT scale");
    if (name.Contains("certainty") || name.Contains("confidence") || name.Contains("certitude")) return ListingOrText(availableFieldTypes, "Certainty");

    if (name.Contains("cardinal") ||
        name == "direction" ||
        name.Contains("bearing")) {
        return ListingOrText(availableFieldTypes, "Cardinal directions");
    }

    // Standard NMEA mappings.
    if (name == "cog" || name.Contains("course over ground")) return "NMEA COG";
    if (name == "sog" || name.Contains("speed over ground")) return "NMEA SOG";
    if (name == "aws" || name.Contains("apparent wind speed")) return "NMEA AWS";
    if (name == "awa" || name.Contains("apparent wind angle")) return "NMEA AWA";
    if (name == "tws" || name.Contains("true wind speed")) return "NMEA TWS";
    if (name == "twd" || name.Contains("true wind direction")) return "NMEA TWD";

    // Boolean-like columns.
    if (name == "yes no" ||
        name == "yes/no" ||
        name == "true false" ||
        name == "true/false" ||
        name == "checked" ||
        name == "checkbox" ||
        name == "bool" ||
        name == "boolean" ||
        name.Contains("present absent") ||
        name.Contains("presence")) {
        return "Checkbox";
    }

    return "Text";
}
