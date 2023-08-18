window.onload = function () {
    /*async function fetchImage(url) {
        let totalSize = 1, rangeStart = 0, image = new Blob()

        while (image.size < totalSize) {
            let response = await fetch(url, {})

            if (response.status === 206) {
                if (totalSize === 0) {
                    let contentRange = response.headers.get("Content-Range")

                    totalSize = parseInt(contentRange.split("/")[1])
                }

                let chunk = await response.blob()

                image = new Blob([image, chunk], {type: "image/png"})

                rangeStart = image.size;
            } else {
                throw new Error(`Unexpected response status: ${response.status}`)
            }
        }

        console.log(totalSize, image.size)

        return URL.createObjectURL(image)
    }

    fetchImage("https://localhost:9999/backgroundImage.png")
        .then(objectURL => {
            document.body.style.backgroundImage = `url(${objectURL})`
        })
        .catch(error => {
            console.error(error)
        })
*/
}
