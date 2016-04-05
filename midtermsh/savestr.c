					char *str = (char *) malloc((i - mark + 1) * sizeof(char));
					memcpy(str, buffer + mark, i - mark + 1);
					str[i - mark] = '\0';
					//printf(" ---- %s\n", str);
					if (one_cmd_size == one_cmd_iter) {
						//char **one_cmd_buffer_tmp = (char **) malloc((one_cmd_size *=2) * sizeof(char *));
						//memcpy(one_cmd_buffer_tmp, one_cmd_buffer, one_cmd_iter);
						//free(one_cmd_buffer);
						//one_cmd_buffer = one_cmd_buffer_tmp;
						expand((void **) &one_cmd_buffer, one_cmd_size * sizeof(char *));
						one_cmd_size *=2;
					}
					one_cmd_buffer[one_cmd_iter++] = str;
					--i;
					while (i < cnt && is_whitespace(buffer[++i + 1]));
					mark = i + 1;
