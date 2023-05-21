#include <memory>
#include <fstream>
#include <stitcher/nlohmann/json.hpp>
#include "stitcher.h"
#include "separation_stitcher.h"
#include "direct_alignment_stitcher.h"
#include "opencv_sift_2d_stitcher.h"
#include "sift_2d_stitcher.h"
#include "sift_3d_stitcher.h"


using AlgoList = std::vector<std::pair<std::shared_ptr<StitcherImpl>, std::string>>;
using json = nlohmann::json;


int main(int argc, char *argv[]) {
    AlgoList stitchers = {
        {std::make_shared<L2DirectAlignmentStitcher>(), "l2_direct_alignment"},
        {std::make_shared<OpenCVSIFT2DStitcher>(), "opencv_sift_2d"},
        {std::make_shared<SIFT2DStitcher>(), "sift_2d"}};
        // {std::make_shared<SIFT3DStitcher>(), "sift_3d"}};

    std::vector<std::string> recons_names = {
        "bicycle_wheel_x512",
        "big_wheel_x512",
        "nonuniform_2_x512",
        "pores_2_x512",
        "big_wheel_x128",
        "nonuniform_2_x128",
        "nonuniform_x256",
    };

    for (std::string& recon_name : recons_names) {
        // Parse JSON parameters
        std::string recon_path = "../testing/" + recon_name;
        std::string recon_src_path = recon_path + "/source/info.json";
        std::ifstream fs(recon_src_path);
        if(!fs) {
            printf("%s %s\n", "Unable to open file", recon_src_path.data());
            continue;
        }

        json data = json::parse(fs, nullptr, false);
        if (data.is_discarded()) {
            printf("Parse error");
            continue;
        }

        int parts_num = data["parts_num"].get<int>();

        // Load reconstructions
        std::vector<std::shared_ptr<VoxelContainer>> recons(parts_num);

        for (int part_id = 0; part_id < parts_num; ++part_id) {
            std::string recon_part_path = recon_path + "/source/" + std::to_string(part_id) + "/info.json";
            recons[part_id] = std::make_shared<VoxelContainer>();
            recons[part_id]->loadFromJson(recon_part_path);
        }

        // Stitch
        for (auto stitcher : stitchers) {
            std::string recon_result_path = recon_path + "/" + stitcher.second;
            auto result_recon = stitcher.first->stitch(recons);
            result_recon->saveToJson(recon_result_path);

            // Dump stitching params
            std::vector<json> params_data_vec(parts_num);
            for (int part_id = 0; part_id < parts_num; ++part_id) {
                auto params = recons[part_id]->getEstStitchParams();
                params_data_vec[part_id]["offset_x"] = params.offsetX;
                params_data_vec[part_id]["offset_y"] = params.offsetY;
                params_data_vec[part_id]["offset_z"] = params.offsetZ;
            }

            json params_data(params_data_vec);
            std::string params_path = recon_result_path + "/params.json";
            std::ofstream pfs(params_path);
            if(!pfs) {
                printf("%s %s\n", "Unable to open file", params_path.data());
                continue;
            }
            pfs << params_data.dump(4) << std::endl;

            // Write slice image
            TiffImage<uint8_t> img;
            result_recon->getSlice<uint8_t>(img, 0, result_recon->getSize().x / 2, true);
            img.saveAs((recon_result_path + ".png").data());
        }        
    }

    return 0;
}
