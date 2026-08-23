# syntax=docker/dockerfile:1

FROM ubuntu:noble@sha256:c4a8d5503dfb2a3eb8ab5f807da5bc69a85730fb49b5cfca2330194ebcc41c7b

ARG DEBIAN_FRONTEND="noninteractive"

COPY apt-packages.in /tmp/apt.in
RUN apt update && \
  grep -vE '^\s*#' /tmp/apt.in | xargs apt install --no-install-recommends -y && \
  rm -rf /var/lib/apt/lists/* && apt-get clean -y && rm /tmp/apt.in && \
  sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && \
  locale-gen

COPY .clang-format /.clang-format
COPY .clangd /.clangd
COPY .cmake-format.json /.cmake-format.json

# Environment:
ENV TZ=Etc/UTC
ENV LC_ALL=en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en

RUN echo "ALL ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

ARG CONTAINER_USER=developer
ARG WORKSPACE_FOLDER=/workspace

RUN if id ubuntu >/dev/null 2>&1; then \
      userdel -r ubuntu && groupdel ubuntu 2>/dev/null || true; \
    fi && \
    useradd -s /bin/bash -m ${CONTAINER_USER} && \
  echo "\nexport PROMPT_COMMAND=\"history -a; history -n\n\"" >> /home/${CONTAINER_USER}/.bashrc

# --- Install ESP-IDF Toolchain (v5.2 or v5.3 supports ESP32-C6) ---
ENV IDF_PATH=/opt/esp/idf
ENV IDF_TOOLS_PATH=/opt/esp/espressif

RUN git clone --recursive -b v5.3 https://github.com/espressif/esp-idf.git ${IDF_PATH} && \
    ${IDF_PATH}/install.sh esp32c6 && \
    echo "source ${IDF_PATH}/export.sh" >> /home/${CONTAINER_USER}/.bashrc

USER ${CONTAINER_USER}