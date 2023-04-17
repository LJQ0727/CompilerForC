#pragma once

#include <vector>
#include <string>

// prepare the `idx_to_token` names, and the feed the scanned tokens to the token ostream
void scanner_driver(std::string input_fname, std::ostream* token_ostream, std::vector<std::string>* idx_to_token_copy, std::ostream* semantic_ostream);
