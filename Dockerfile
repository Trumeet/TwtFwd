FROM docker.io/alpine:latest AS building

WORKDIR /root
ADD . /root/source

RUN apk add build-base autoconf automake curl-dev json-c-dev && \
	cd source && \
	sh ./autogen.sh && \
	mkdir ../build && \
	./configure --prefix="/root/build/" && \
	make && \
	make install

FROM docker.io/alpine:latest AS runtime
WORKDIR /
COPY --from=0 /root/build/ ./usr/local/
RUN apk add --no-cache curl json-c
ENTRYPOINT [ "/usr/local/bin/twtfwd" ]
