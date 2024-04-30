#include "MimeTypes.hpp"

using namespace ORION;

const std::string MimeTypes::OCTET_STREAM = "application/octet-stream";

const std::map<std::string, std::string> MimeTypes::MIME_TYPES_MAP = {
    // Common Text Files
    { "txt", "text/plain" },                // Plain Text
    { "html", "text/html" },                // HTML
    { "htm", "text/html" },                 // HTML
    { "css", "text/css" },                  // Cascading Style Sheets
    { "js", "text/javascript" },            // JavaScript
    { "json", "application/json" },         // JSON
    { "xml", "application/xml" },           // XML
    { "xhtml", "application/xhtml+xml" },   // XHTML
    { "csv", "text/csv" },                  // Comma-Separated Values
    { "rtf", "application/rtf" },           // Rich Text Format
    { "tsv", "text/tab-separated-values" }, // Tab-Separated Values
    { "markdown", "text/markdown" },        // Markdown
    { "md", "text/markdown" },              // Markdown
    { "ini", "text/plain" },                // INI
    // Common Audio Files
    { "mp3", "audio/mpeg" },     // MP3 Audio
    { "wav", "audio/wav" },      // WAV Audio
    { "flac", "audio/flac" },    // FLAC Audio
    { "aac", "audio/aac" },      // AAC Audio
    { "opus", "audio/opus" },    // Opus Audio
    { "pcm", "audio/pcm" },      // PCM Audio
    { "m3u", "audio/mpegurl" },  // MP3 Audio
    { "m3u8", "audio/mpegurl" }, // MP3 Audio
    // Common Video Files
    { "mp4", "video/mp4" },        // MP4 Video
    { "webm", "video/webm" },      // WebM Video
    { "mkv", "video/x-matroska" }, // Matroska Video
    { "flv", "video/x-flv" },      // Flash Video
    { "avi", "video/x-msvideo" },  // AVI Video
    { "mov", "video/quicktime" },  // QuickTime Video
    { "wmv", "video/x-ms-wmv" },   // Windows Media Video
    { "mpeg", "video/mpeg" },      // MPEG Video
    { "mpg", "video/mpeg" },       // MPEG Video
    { "m4v", "video/x-m4v" },      // M4V Video
    // Common Image Files
    { "png", "image/png" },      // Portable Network Graphics
    { "jpg", "image/jpeg" },     // JPEG Image
    { "jpeg", "image/jpeg" },    // JPEG Image
    { "gif", "image/gif" },      // Graphics Interchange Format
    { "bmp", "image/bmp" },      // Bitmap Image
    { "tiff", "image/tiff" },    // Tagged Image File Format
    { "tif", "image/tiff" },     // Tagged Image File Format
    { "webp", "image/webp" },    // WebP Image
    { "ico", "image/x-icon" },   // Icon Image
    { "svg", "image/svg+xml" },  // Scalable Vector Graphics
    { "svgz", "image/svg+xml" }, // Scalable Vector Graphics
    // Common Archive Files
    { "zip", "application/zip" },              // ZIP Archive
    { "rar", "application/x-rar-compressed" }, // RAR Archive
    { "7z", "application/x-7z-compressed" },   // 7-Zip Archive
    { "tar", "application/x-tar" },            // Tape Archive
    { "gz", "application/gzip" },              // Gzip Archive
    { "bz2", "application/x-bzip2" },          // Bzip2 Archive
    // Common Document Files
    { "pdf", "application/pdf" },                                                            // Portable Document Format
    { "doc", "application/msword" },                                                         // Microsoft Word
    { "docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document" },   // Microsoft Word
    { "xls", "application/vnd.ms-excel" },                                                   // Microsoft Excel
    { "xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" },         // Microsoft Excel
    { "ppt", "application/vnd.ms-powerpoint" },                                              // Microsoft PowerPoint
    { "pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation" }, // Microsoft PowerPoint
    { "odt", "application/vnd.oasis.opendocument.text" },                                    // OpenDocument Text
    { "ods", "application/vnd.oasis.opendocument.spreadsheet" },                             // OpenDocument Spreadsheet
};