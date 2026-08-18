#include "graphics/model_importer.h"

#include "core/io/file.h"
#include "core/io/file_system.h"

namespace golias {

    Ref<ModelImporter> CreateGltfImporter();
    Ref<ModelImporter> CreateObjImporter();

    Ref<ModelImporter> ModelImporter::ForPath(const String& virtualPath) {
        const String extension = file::Extension(virtualPath);
        if (extension == ".gltf" || extension == ".glb") {
            return CreateGltfImporter();
        }

        if (extension == ".obj") {
            return CreateObjImporter();
        }

        return nullptr;
    }

    bool ModelImporter::ImportFile(const String& virtualPath, ImportedModel& output, String& error) {
        const Ref<ModelImporter> importer = ForPath(virtualPath);
        if (!importer) {
            error = "Unsupported model format '" + virtualPath + "'.";
            return false;
        }
        
        return importer->Import(virtualPath, output, error);
    }

} // namespace golias
