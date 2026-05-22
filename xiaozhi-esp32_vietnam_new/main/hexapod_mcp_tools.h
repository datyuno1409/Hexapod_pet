#ifndef HEXAPOD_MCP_TOOLS_H
#define HEXAPOD_MCP_TOOLS_H

#include "mcp_server.h"
#include <string>

/**
 * @brief Hexapod MCP Tools
 *
 * Provides MCP tools for controlling hexapod robot from VoiceBot.
 * Tools include motion control, camera capture, and emotion display.
 */
class HexapodMcpTools {
public:
    /**
     * @brief Register all hexapod MCP tools
     * @param mcp_server Reference to MCP server instance
     */
    static void RegisterTools(McpServer& mcp_server);

private:
    /**
     * @brief hexapod.move tool handler
     * Moves the hexapod in different directions
     */
    static ReturnValue HandleMoveCommand(const PropertyList& args);

    /**
     * @brief hexapod.camera.capture tool handler
     * Captures image from hexapod camera
     */
    static ReturnValue HandleCameraCapture(const PropertyList& args);

    /**
     * @brief hexapod.camera.stream tool handler
     * Controls camera streaming
     */
    static ReturnValue HandleCameraStream(const PropertyList& args);

    /**
     * @brief hexapod.emotion tool handler
     * Sets emotion display on hexapod
     */
    static ReturnValue HandleEmotion(const PropertyList& args);

    /**
     * @brief hexapod.status tool handler
     * Gets hexapod status
     */
    static ReturnValue HandleStatus(const PropertyList& args);
};

#endif  // HEXAPOD_MCP_TOOLS_H
