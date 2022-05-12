FROM gcc:latest
COPY ./src /usr/src/ipxe
WORKDIR /usr/src/ipxe
RUN apt update -y && apt install isolinux genisoimage gcc-multilib -y