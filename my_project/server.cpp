#include "crow_all.h"
#include <torch/script.h>  // For torch::jit::load
#include <iostream>

int main() {
    crow::SimpleApp app;

    // Assuming the model is loaded globally if it's intended to be kept in memory
    // and used across multiple requests.
    torch::jit::script::Module model;

    // This route loads the model. It should be called once to initialize the model.
    CROW_ROUTE(app, "/load_model")([&](const crow::request& req){
        auto model_path = req.url_params.get("model_path");
        if (!model_path) {
            return crow::response(400, "Model path parameter is missing");
        }

        try {
            // Load the PyTorch model weights
            model = torch::jit::load(model_path);
            std::cout << "Model loaded successfully." << std::endl;

            return crow::response("Model loaded and ready for inference");
        } catch (const c10::Error& e) {
            return crow::response(500, "Failed to load model: " + std::string(e.what()));
        }
    });

    // This route could perform inference using the loaded model
    CROW_ROUTE(app, "/infer")([&](const crow::request& req){
        try {
            // Example of performing inference
            // Create a dummy input tensor
            auto input = torch::rand({1, 3, 224, 224});
            auto output = model.forward({input}).toTensor();

            // Just for demonstration, print the output tensor
            std::cout << "Inference output: " << output.slice(/*dim=*/1, /*start=*/0, /*end=*/5) << std::endl;

            return crow::response("Inference performed successfully");
        } catch (const c10::Error& e) {
            return crow::response(500, "Inference failed: " + std::string(e.what()));
        }
    });

    app.port(8080).multithreaded().run();
}
